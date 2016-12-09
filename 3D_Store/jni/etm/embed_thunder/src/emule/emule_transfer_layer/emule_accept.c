#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_accept.h"
#include "emule_tcp_device.h"
#include "../emule_pipe/emule_pipe_device.h"
#include "../emule_utility/emule_peer.h"
#include "../emule_impl.h"
#include "socket_proxy_interface.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"
#include "utility/list.h"

// 这个socket是emule协议本身的socket，TCP socket
static  SOCKET		g_emule_accept_socket;

// 用于保存全部的被动连接peer
static  LIST        g_emule_passive_peers;

_int32 emule_create_acceptor(void)
{
	_int32 ret = SUCCESS;
	SD_SOCKADDR addr;
	EMULE_PEER* local_peer = emule_get_local_peer();

    LOG_DEBUG("emule_create_acceptor.");

	ret = socket_proxy_create(&g_emule_accept_socket, SD_SOCK_STREAM);
	CHECK_VALUE(ret);
	addr._sin_family = SD_AF_INET;
	addr._sin_addr = ANY_ADDRESS;
	addr._sin_port = sd_htons(4662);
	ret = socket_proxy_bind(g_emule_accept_socket, &addr);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emule_create_acceptor bind port failed, port = %u", sd_ntohs(addr._sin_port));
		return socket_proxy_close(g_emule_accept_socket);
	}
	local_peer->_tcp_port = sd_ntohs(addr._sin_port);
    LOG_DEBUG("local_peer->_tcp_port = %hu.", local_peer->_tcp_port);

	ret = socket_proxy_listen(g_emule_accept_socket, 10);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("emule_create_acceptor bind port failed, port = %u", sd_ntohs(addr._sin_port));
		return socket_proxy_close(g_emule_accept_socket);
	}

    list_init(&g_emule_passive_peers);

	return socket_proxy_accept(g_emule_accept_socket, emule_notify_accept, NULL);
}

_int32 emule_close_acceptor(void)
{
	_int32 ret = SUCCESS;
	_u32 count = 0;

    LOG_DEBUG("emule_close_acceptor.");

	ret = socket_proxy_peek_op_count(g_emule_accept_socket, DEVICE_SOCKET_TCP, &count);
    if(count != 0) {
        LOG_DEBUG("emule_close_acceptor, socket_proxy_peek_op_count = %d is not zero.", count);
        return ret;
    }
	ret = socket_proxy_close(g_emule_accept_socket);
	g_emule_accept_socket = INVALID_SOCKET;

    if (list_size(&g_emule_passive_peers) != 0)
    {
        list_clear(&g_emule_passive_peers);
    }
	return ret;
}

_int32 emule_notify_accept(_int32 errcode, _u32 pending_op_count, SOCKET conn_sock, void* user_data)
{
	_int32 ret = SUCCESS;
    EMULE_PEER *remote_peer = NULL;
	EMULE_TCP_DEVICE* tcp_device = NULL;
	EMULE_PIPE_DEVICE* pipe_device = NULL;

    LOG_DEBUG("emule_notify_accept, errcode = %d, pending_op_count = %u, conn_sock = %u, user_data = 0x%x.",
        errcode, pending_op_count, conn_sock, user_data);

	if(errcode != SUCCESS)
	{
        LOG_ERROR("emule_notify_accept error(%d).", errcode);
        emule_close_acceptor();
	}

    ret = sd_malloc(sizeof(EMULE_PEER), (void *)&remote_peer);
    CHECK_VALUE(ret);
    sd_memset((void *)remote_peer, 0, sizeof(remote_peer));

    emule_peer_init(remote_peer);

    ret = list_push(&g_emule_passive_peers, (void *)remote_peer);
    CHECK_VALUE(ret);

	ret = emule_pipe_device_create(&pipe_device);
    CHECK_VALUE(ret);
	if(ret != SUCCESS)
    {
        emule_pipe_device_destroy(pipe_device);
		return socket_proxy_accept(g_emule_accept_socket, emule_notify_accept, NULL);
    }
    pipe_device->_passive = TRUE;

	ret = emule_socket_device_create(&pipe_device->_socket_device, EMULE_TCP_TYPE, NULL, emule_pipe_device_get_callback_table(), pipe_device);
	if(ret != SUCCESS)
	{
		emule_pipe_device_destroy(pipe_device);
		return socket_proxy_accept(g_emule_accept_socket, emule_notify_accept, NULL);
	}
	tcp_device = (EMULE_TCP_DEVICE*)pipe_device->_socket_device->_device;
	tcp_device->_sock = conn_sock;

	ret = emule_notify_passive_connect(pipe_device);
	if(ret != SUCCESS)
		emule_pipe_device_close(pipe_device);
	
    return socket_proxy_accept(g_emule_accept_socket, emule_notify_accept, NULL);
}


#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

