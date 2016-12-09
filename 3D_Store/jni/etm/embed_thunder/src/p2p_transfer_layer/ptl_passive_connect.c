#include "ptl_passive_connect.h"
#include "ptl_utility.h"
#include "ptl_interface.h"
#include "ptl_udp_device.h"
#include "ptl_cmd_define.h"
#include "ptl_cmd_extractor.h"
#include "ptl_cmd_sender.h"
#include "ptl_callback_func.h"
#include "ptl_active_udp_broker.h"
#include "ptl_passive_udp_broker.h"
#include "ptl_active_tcp_broker.h"
#include "ptl_passive_tcp_broker.h"
#include "tcp/tcp_interface.h"
#include "tcp/tcp_memory_slab.h"
#include "udt/udt_interface.h"
#include "udt/udt_cmd_define.h"
#include "udt/udt_device.h"
#include "udt/udt_utility.h"
#include "udt/udt_memory_slab.h"
#include "udt/udt_device_manager.h"
#include "udt/udt_impl.h"
#include "socket_proxy_interface.h"
#include "utility/local_ip.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/map.h"
#include "utility/settings.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_P2P_TRANSFER_LAYER
#include "utility/logger.h"

#define P2P_OLD_PROTOCOL_VERSION 41

static SET g_ptl_accept_data_set;
static void* ptl_passive_connect_table[6] = {(void*)ptl_passive_connect_callback, (void*)ptl_passive_send_callback, (void*)ptl_passive_recv_callback, NULL, NULL, (void*)ptl_passive_close_callback};
static SOCKET g_ptl_tcp_accept_sock = INVALID_SOCKET;
static p2p_accept_func g_p2p_accept = NULL;

_int32 ptl_accept_data_comparator(void* E1, void* E2)
{
	if(E1 == E2)
		return 0;
	else if(E1 > E2)
		return 1;
	else
		return -1;
}

void ptl_regiest_p2p_accept_func(p2p_accept_func func)
{
	g_p2p_accept = func;
}

_int32 ptl_create_passive_connect(void)
{
	LOG_DEBUG("ptl_create_passive_connect.");
	set_init(&g_ptl_accept_data_set, ptl_accept_data_comparator);
	//create tcp accept
	_int32 ret = socket_proxy_create(&g_ptl_tcp_accept_sock, SD_SOCK_STREAM);
	CHECK_VALUE(ret);
	LOG_DEBUG("ptl_create_passive_connect g_ptl_tcp_accept_sock = %u", g_ptl_tcp_accept_sock);
	_u32 tcp_port = 1080;
	settings_get_int_item("ptl_setting.tcp_port", (_int32*)&tcp_port);
	SD_SOCKADDR addr;
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ANY_ADDRESS;		/*any address*/
	addr._sin_port = sd_htons((_u16)tcp_port);
	ret = socket_proxy_bind(g_ptl_tcp_accept_sock, &addr);
	if (ret != SUCCESS)
	{
		socket_proxy_close(g_ptl_tcp_accept_sock);
		g_ptl_tcp_accept_sock = INVALID_SOCKET;
		return ret;
	}
	ret = socket_proxy_listen(g_ptl_tcp_accept_sock, 10);
	if (ret != SUCCESS)
	{
		socket_proxy_close(g_ptl_tcp_accept_sock);
		g_ptl_tcp_accept_sock = INVALID_SOCKET;
		return ret;
	}
	ptl_set_local_tcp_port(sd_ntohs(addr._sin_port));
	ret = socket_proxy_accept(g_ptl_tcp_accept_sock, ptl_accept_passive_tcp_connect, NULL);
	if(ret != SUCCESS)
	{
		socket_proxy_close(g_ptl_tcp_accept_sock);
		g_ptl_tcp_accept_sock = INVALID_SOCKET;
		return ret;
	}
	LOG_DEBUG("ptl_create_passive_connect success, bind tcp port = %u.", sd_ntohs(addr._sin_port));
	return ret;
}

_int32 ptl_close_passive_connect(void)
{	//这已经在反初始化，尽量回收内存
	_u32 op_count = 0;
	SET_ITERATOR  cur_iter = NULL, next_iter = NULL;
	PTL_ACCEPT_DATA* data = NULL;
	LOG_DEBUG("ptl_close_passive_connect, g_ptl_accept_data_set size = %u.", set_size(&g_ptl_accept_data_set));
	cur_iter = SET_BEGIN(g_ptl_accept_data_set);
	while(cur_iter != SET_END(g_ptl_accept_data_set))
	{
		next_iter = SET_NEXT(g_ptl_accept_data_set, cur_iter);
		data = (PTL_ACCEPT_DATA*)SET_DATA(cur_iter);
		ptl_erase_ptl_accept_data(data);
		cur_iter = next_iter;
	}
	//记得全局变量要清空
	set_clear(&g_ptl_accept_data_set);
	if(g_ptl_tcp_accept_sock == INVALID_SOCKET)
		return SUCCESS;
	socket_proxy_peek_op_count(g_ptl_tcp_accept_sock, DEVICE_SOCKET_TCP, &op_count);
	sd_assert(op_count == 1);
	return socket_proxy_cancel(g_ptl_tcp_accept_sock, DEVICE_SOCKET_TCP);		//在callback中关闭g_accept_socket
}

//NODE:jieouy这里退出可能会有问题，在etm的线程中直接对et的g_accept_sock进行操作。
_int32 force_close_p2p_acceptor_socket(void)
{
      if(g_ptl_tcp_accept_sock == INVALID_SOCKET)
		return SUCCESS;   
	//socket_proxy_close(g_ptl_tcp_accept_sock);
	g_ptl_tcp_accept_sock = INVALID_SOCKET;     
	return SUCCESS;   
}

_int32 ptl_erase_ptl_accept_data(PTL_ACCEPT_DATA* data)
{
	_int32 ret = SUCCESS;
	UDT_DEVICE* udt_device = NULL;
	//PASSIVE_UDP_BROKER_DATA* broker_data = NULL;
	LOG_DEBUG("[device = 0x%x]ptl_erase_ptl_accept_data, data = 0x%x.", data->_device, data);
	ret = set_erase_node(&g_ptl_accept_data_set, data);
	CHECK_VALUE(ret);
	if(data->_device != NULL)
	{
		if(data->_device->_connect_type == PASSIVE_UDP_BROKER_CONN && data->_device->_device != NULL)
		{	//如果是被动udp broker 连接
			udt_device = (UDT_DEVICE*)data->_device->_device;

			//broker_data = ptl_find_passive_udp_broker_data(udt_device->_id._peerid_hashcode, data->_broker_seq);
			//ptl_erase_passive_udp_broker_data(broker_data);

			ptl_delete_passive_udp_broker_data(udt_device->_id._peerid_hashcode, data->_broker_seq);
		}
		ret = ptl_close_device(data->_device);
		if(ret == SUCCESS)
			ptl_destroy_device(data->_device);
	}
	return sd_free(data);
}

_int32 ptl_accept_passive_tcp_connect(_int32 errcode, _u32 pending_op_count, SOCKET conn_sock, void* user_data)
{
#ifdef _DEBUG
	SD_SOCKADDR addr;
	char buffer[24] = {0};
#endif
	_int32 ret = SUCCESS;
	PTL_ACCEPT_DATA* data = NULL;
	if(errcode == MSG_CANCELLED)
	{
		sd_assert(pending_op_count == 0);		//这个socket上面只有一个accept操作,所以cancel后pending_op_count为0
		ret = socket_proxy_close(g_ptl_tcp_accept_sock);
		g_ptl_tcp_accept_sock = INVALID_SOCKET;		 
		return SUCCESS;
	}
	if(errcode != SUCCESS)
	{
		LOG_DEBUG("ptl_accept_passive_tcp_connect, but failed, errcode = %d.", errcode);
		return socket_proxy_accept(g_ptl_tcp_accept_sock, ptl_accept_passive_tcp_connect, NULL);
	}
	//accept success
#ifdef _DEBUG
	ret = sd_getpeername(conn_sock, &addr);
    if( ret == SUCCESS )
    {
	sd_inet_ntoa(addr._sin_addr, buffer, 24);
	LOG_DEBUG("ptl_accept_passive_tcp_connect, socket = %u, remote_ip = %s, remote_port = %u.", conn_sock, buffer, addr._sin_port);
    }
#endif
	ret = sd_malloc(sizeof(PTL_ACCEPT_DATA), (void**)&data);
	if(ret != SUCCESS)
	{
		socket_proxy_close(conn_sock);
		return socket_proxy_accept(g_ptl_tcp_accept_sock, ptl_accept_passive_tcp_connect, NULL);
	}
	sd_memset(data, 0, sizeof(PTL_ACCEPT_DATA));
	ret = ptl_create_device(&data->_device, data, ptl_passive_connect_table);
	if(ret != SUCCESS)
	{
		socket_proxy_close(conn_sock);
		sd_free(data);
		data = NULL;
		return socket_proxy_accept(g_ptl_tcp_accept_sock, ptl_accept_passive_tcp_connect, NULL);
	}
	ret = tcp_device_create((TCP_DEVICE**)&data->_device->_device, conn_sock, data->_device);
	if(ret != SUCCESS)
	{
		socket_proxy_close(conn_sock);
		ptl_destroy_device(data->_device);
		sd_free(data);
		data = NULL;
		return socket_proxy_accept(g_ptl_tcp_accept_sock, ptl_accept_passive_tcp_connect, NULL);
	}
	data->_device->_type = TCP_TYPE;
	data->_device->_connect_type = PASSIVE_TCP_CONN;
	ret = set_insert_node(&g_ptl_accept_data_set, data);
	CHECK_VALUE(ret);
	ptl_passive_connect_callback(SUCCESS, data->_device);
	return socket_proxy_accept(g_ptl_tcp_accept_sock, ptl_accept_passive_tcp_connect, NULL);
}

_int32 ptl_accept_passive_tcp_broker_connect(PASSIVE_TCP_BROKER_DATA* broker_data)
{
#ifdef _DEBUG
	SD_SOCKADDR addr;
	char buffer[24] = {0};
#endif
	_int32 ret = SUCCESS;
	PTL_ACCEPT_DATA* data = NULL;
#ifdef _DEBUG
	ret = sd_getpeername(broker_data->_sock, &addr);
    if(ret == SUCCESS)
    {
	sd_inet_ntoa(addr._sin_addr, buffer, 24);
	LOG_DEBUG("ptl_accept_passive_tcp_broker_connect, socket = %u, remote_ip = %s, remote_port = %u.", broker_data->_sock, buffer, (_u32)addr._sin_port);
    }
#endif
	ret = sd_malloc(sizeof(PTL_ACCEPT_DATA), (void**)&data);
	CHECK_VALUE(ret);
	sd_memset(data, 0, sizeof(PTL_ACCEPT_DATA));
	ret = ptl_create_device(&data->_device, data, ptl_passive_connect_table);
	if(ret != SUCCESS)
	{
		return sd_free(data);
	}
	ret = tcp_device_create((TCP_DEVICE**)&data->_device->_device, broker_data->_sock, data->_device);
	if(ret != SUCCESS)
	{
		ptl_destroy_device(data->_device);
		return sd_free(data);
	}
	data->_device->_type = TCP_TYPE;
	data->_device->_connect_type = PASSIVE_TCP_BROKER_CONN;
	ret = set_insert_node(&g_ptl_accept_data_set, data);
	CHECK_VALUE(ret);
	ptl_passive_connect_callback(SUCCESS, data->_device);
	broker_data->_sock = INVALID_SOCKET;			//这个socket已经被接管了
	return ret;
}

_int32 ptl_accept_passive_udp_broker_connect(PASSIVE_UDP_BROKER_DATA* broker_data)
{
	_int32 ret = SUCCESS;
	PTL_ACCEPT_DATA* data = NULL;
	ret = sd_malloc(sizeof(PTL_ACCEPT_DATA), (void**)&data);
	if(ret != SUCCESS)
	{
		LOG_ERROR("ptl_accept_passive_udp_broker_connect, but sd_malloc PTL_ACCEPT_DATA failed, errcode = %d.", ret);
		return ret;
	}
	sd_memset(data, 0, sizeof(PTL_ACCEPT_DATA));
	ret = ptl_create_device(&data->_device, data, ptl_passive_connect_table);
	if(ret != SUCCESS)
	{
		LOG_ERROR("ptl_accept_passive_udp_broker_connect, but ptl_create_device failed, errcode = %d.", ret);
		sd_free(data);
		data = NULL;
		return ret;
	}
	data->_device->_type = UDT_TYPE;
	data->_device->_connect_type = PASSIVE_UDP_BROKER_CONN;
	ret = udt_device_create((UDT_DEVICE**)&data->_device->_device, udt_generate_source_port(), broker_data->_seq, udt_hash_peerid(broker_data->_remote_peerid), data->_device);
	if(ret != SUCCESS)
	{
		LOG_ERROR("ptl_accept_passive_udp_broker_connect, but udt_device_create failed, errcode = %d.", ret);
		ptl_destroy_device(data->_device);
		sd_free(data);
		data = NULL;
		return ret;
	}
	udt_add_device((UDT_DEVICE*)data->_device->_device);
	LOG_DEBUG("[device = 0x%x]ptl_accept_passive_udp_broker_connect, create udt_device.", data->_device);
	data->_broker_seq = broker_data->_seq;
	udt_device_connect((UDT_DEVICE*)data->_device->_device, broker_data->_ip, broker_data->_udp_port);
	ret = set_insert_node(&g_ptl_accept_data_set, data);
	CHECK_VALUE(ret);
	return ret;
}


_int32 ptl_accept_udt_passvie_connect(SYN_CMD* cmd,  _u32 remote_ip, _u16 remote_port)
{
	_int32 ret = SUCCESS;
	ACTIVE_UDP_BROKER_DATA* broker_data = NULL;
	PTL_ACCEPT_DATA* data = NULL;	
	LOG_DEBUG("ptl_accept_udt_passvie_connect, conn_id[%u, %u, %u], seq = %u, ack = %u, peerid_hashcode = %u, udt_version = %u.", 
				cmd->_target_port, cmd->_source_port, cmd->_peerid_hashcode, cmd->_seq_num, cmd->_ack_num, cmd->_peerid_hashcode, cmd->_udt_version);
	//查看一下这个被动连接是否active_udp_broker
	if(cmd->_target_port != 0 && sd_is_in_nat(sd_get_local_ip()) == FALSE)
	{
		broker_data = ptl_find_active_udp_broker_data(cmd->_target_port);
		if(broker_data == NULL)
		{
			LOG_ERROR("ptl_accept_udt_passvie_connect, but can't find active_udp_broker_data.");
			return SUCCESS;
		}
		if(broker_data->_device == NULL)
		{	//这个 broker_data请求已经被cancel掉了
			LOG_DEBUG("ptl_accept_udt_passvie_connect, udp broker request is cancel, broker_seq = %u.", broker_data->_seq_num);
			return SUCCESS;
		}
		ret = udt_device_create((UDT_DEVICE**)&broker_data->_device->_device, cmd->_target_port, cmd->_source_port, cmd->_peerid_hashcode, broker_data->_device);
		CHECK_VALUE(ret);
		udt_add_device((UDT_DEVICE*)broker_data->_device->_device);
		LOG_DEBUG("[device = 0x%x]ptl_accept_udt_passvie_connect, this is a active udp broker connect, create udt_device, conn_id[%u, %u, %u].",
					broker_data->_device, cmd->_target_port, cmd->_source_port,  cmd->_peerid_hashcode);
		broker_data->_device->_type = UDT_TYPE;
		sd_assert(broker_data->_device->_connect_type == ACTIVE_UDP_BROKER_CONN);
		ret = udt_passive_connect((UDT_DEVICE*)broker_data->_device->_device, cmd, remote_ip, remote_port);
		CHECK_VALUE(ret);
		return ptl_erase_active_udp_broker_data(broker_data);
	}
	//这是一个普通的udp被动连接
	ret = sd_malloc(sizeof(PTL_ACCEPT_DATA), (void**)&data);
	if(ret != SUCCESS)
	{
		LOG_ERROR("ptl_accept_udt_passvie_connect, but sd_malloc PTL_ACCEPT_DATA failed, errcode = %d.", ret);
		return ret;
	}
	sd_memset(data, 0, sizeof(PTL_ACCEPT_DATA));
	ret = ptl_create_device(&data->_device, data, ptl_passive_connect_table);
	if(ret != SUCCESS)
	{
		LOG_ERROR("ptl_accept_udt_passvie_connect, but ptl_create_device failed, errcode = %d.", ret);
		sd_free(data);
		data = NULL;
		return ret;
	}
	data->_device->_type = UDT_TYPE;
	if(sd_is_in_nat(sd_get_local_ip()) == TRUE)
		data->_device->_connect_type = PASSIVE_PUNCH_HOLE_CONN;
	else
		data->_device->_connect_type = PASSIVE_UDT_CONN;
	ret = udt_device_create((UDT_DEVICE**)&data->_device->_device, cmd->_target_port, cmd->_source_port, cmd->_peerid_hashcode, data->_device);
	if(ret != SUCCESS)
	{
		LOG_ERROR("ptl_accept_udt_passvie_connect, but udt_device_create failed, errcode = %d.", ret);
		ptl_destroy_device(data->_device);
		sd_free(data);
		data = NULL;
		return ret;
	}
	udt_add_device((UDT_DEVICE*)data->_device->_device);
	LOG_DEBUG("[device = 0x%x]ptl_accept_udt_passvie_connect, create udt_device, conn_id[%u, %u, %u].", data->_device,
				cmd->_target_port, cmd->_source_port,  cmd->_peerid_hashcode);
	udt_passive_connect((UDT_DEVICE*)data->_device->_device, cmd, remote_ip, remote_port);
	ret = set_insert_node(&g_ptl_accept_data_set, data);
	CHECK_VALUE(ret);
	return ret;
}

_int32 ptl_passive_connect_callback(_int32 errcode, PTL_DEVICE* device)
{	
	_int32 ret = SUCCESS;
	PTL_ACCEPT_DATA* data = (PTL_ACCEPT_DATA*)device->_user_data;
	LOG_DEBUG("[device = 0x%x]ptl_passive_connect_callback, errcode = %d.", device, errcode);
	if(errcode != SUCCESS)
	{
		return ptl_erase_ptl_accept_data(data);
	}
	ret = ptl_recv_cmd(device, data->_buffer, PTL_PASSIVE_HEADER_LEN, PTL_PASSIVE_BUFFER_LEN);
	CHECK_VALUE(ret);
	return ret;
}

_int32 ptl_passive_recv_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_recv)
{
	_int32 ret = SUCCESS;
	ACTIVE_TCP_BROKER_DATA* tcp_broker = NULL;
	PTL_ACCEPT_DATA* data = (PTL_ACCEPT_DATA*)device->_user_data;
	TRANSFER_LAYER_CONTROL_CMD cmd;
	_u32 version = 0, data_len = 0;
	_u8 protocol = 0;
	LOG_DEBUG("[device = 0x%x]ptl_passive_recv_callback, errcode = %d, had_recv = %u.", device, errcode, had_recv);
	if(errcode != SUCCESS)
	{
		return ptl_erase_ptl_accept_data(data);
	}
	//接收成功了
	data->_offset += had_recv;
	sd_assert(data->_offset >= PTL_PASSIVE_HEADER_LEN);
	//解析p2p的头部
	sd_memcpy(&version, data->_buffer, sizeof(_u32));			
	sd_memcpy(&data_len, data->_buffer + 4, sizeof(_u32));
	sd_memcpy(&protocol, data->_buffer + 8, sizeof(_u8));
	if(version <= P2P_OLD_PROTOCOL_VERSION || version > 255 || (is_p2p_cmd_valid(protocol) == -1))
	{	
		LOG_DEBUG("[device = 0x%x]invalid p2p, version:%u, cmd:%u, not support, close device.", 
			device, version, protocol);
		return ptl_erase_ptl_accept_data(data);
	}

#if defined(_LOGGER) && defined(_DEBUG)
	{
		LOG_DEBUG("version:%u, data_len:%u, data->offset:%u, protocol:%u.", 
			version, data_len, data->_offset, protocol);
		unsigned char tmp_buffer[512] = {0};
		_u32 i;
		for( i = 0; i < data_len;)
		{
			char hexbuff[4] = {0};
			char2hex(data->_buffer[i], hexbuff, 4);

			sd_strcat(tmp_buffer, hexbuff, 2);
			sd_strcat(tmp_buffer, " ", 1);
			i++;
			if(i % 16 ==0)
			{
				LOG_DEBUG("%s", tmp_buffer);
				memset(tmp_buffer, 0, sizeof(tmp_buffer));
			}
		}

		LOG_DEBUG("%s", tmp_buffer);
	}
#endif

	data_len += (PTL_PASSIVE_HEADER_LEN - 1);
	if(data_len > data->_offset)
	{	//还没收完整，继续收剩余的字节
		return ptl_recv_cmd(device, data->_buffer + data->_offset, data_len - data->_offset, PTL_PASSIVE_BUFFER_LEN - data->_offset);
	}
	//接收到完整的p2p层数据包，查看一下
	if(protocol == TRANSFER_LAYER_CONTROL)
	{	//这是一个tcp_broker连接
		ret = ptl_extract_transfer_layer_control_cmd(data->_buffer, data->_offset,  &cmd);
		if(ret != SUCCESS)
			return ptl_erase_ptl_accept_data(data);
		data->_broker_seq = cmd._broker_seq;
		tcp_broker = ptl_find_active_tcp_broker_data(cmd._broker_seq);
		if(tcp_broker == NULL)
		{
			LOG_DEBUG("ptl_passive_recv_callback, but can't find any active_tcp_broker_data.");
			return ptl_erase_ptl_accept_data(data);
		}
		return ptl_send_transfer_layer_control_resp_cmd(data->_device, 1);
	}
	//接收到了完整的p2p层数据包，提交给上层处理
	g_p2p_accept(&data->_device, data->_buffer, data->_offset);
	//到目前为止数据已经提交给p2p处理了，如果正确处理data->_device会被设置为NULL,
	//表示该对象已经被上层接管，这里只需要把data释放掉就行了
	return ptl_erase_ptl_accept_data(data);
}

_int32 ptl_passive_send_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_send)
{
	ACTIVE_TCP_BROKER_DATA* tcp_broker = NULL;
	PTL_ACCEPT_DATA* data = (PTL_ACCEPT_DATA*)device->_user_data;
	LOG_DEBUG("ptl_passive_send_callback, had_send = %u.", had_send);
	if(errcode != SUCCESS)
		return ptl_erase_ptl_accept_data(data);
	tcp_broker = ptl_find_active_tcp_broker_data(data->_broker_seq);
	if(tcp_broker == NULL)
	{
		LOG_DEBUG("ptl_passive_send_callback, but can't find any active_tcp_broker_data.");
		return ptl_erase_ptl_accept_data(data);
	}
	if(tcp_broker->_device == NULL)
	{
		LOG_DEBUG("ptl_passive_send_callback, but tcp_broker is cancel, broker_seq = %u.", tcp_broker->_seq_num);
		return ptl_erase_ptl_accept_data(data);
	}
	tcp_broker->_device->_type = TCP_TYPE;
	tcp_broker->_device->_device = data->_device->_device;
	//这里正确设置了tcp_device 的回调函数，比较别扭，以后有时间优化
	((TCP_DEVICE*)(tcp_broker->_device->_device))->_device = tcp_broker->_device;		
	//已经被接管了,记得要正确设置类型为UNKNOWN_TYPE,这样后面才能正确删除
	data->_device->_type = UNKNOWN_TYPE;
	data->_device->_device = NULL;		
	ptl_connect_callback(SUCCESS, tcp_broker->_device);
	ptl_erase_active_tcp_broker_data(tcp_broker);
	return ptl_erase_ptl_accept_data(data);
}


_int32 ptl_passive_close_callback(PTL_DEVICE* device)
{
	LOG_DEBUG("[device = 0x%x]ptl_passive_close_callback.", device);
	return ptl_destroy_device(device);
}


