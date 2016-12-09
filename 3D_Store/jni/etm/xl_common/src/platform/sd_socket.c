#include "platform/sd_socket.h"
#include "platform/sd_customed_interface.h"
#include "utility/errcode.h"
#include "utility/define.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "platform/sd_network.h"

#if defined(LINUX)
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#if defined(_ANDROID_LINUX)
#include <net/if.h>
#include <net/if_arp.h>
#endif

#elif defined(WINCE)
//#include <errno.h>
//#include <fcntl.h>
//#include <sys/types.h>
#include <winsock2.h>
#endif


#define LOGID LOGID_INTERFACE
#include "utility/logger.h"

#define CONV_FROM_SD_SOCKADDR(addr, psd_addr)  {(addr).sin_family = (psd_addr)->_sin_family; (addr).sin_port = (psd_addr)->_sin_port; (addr).sin_addr.s_addr = (psd_addr)->_sin_addr;}
#define CONV_TO_SD_SOCKADDR(psd_addr, addr)  {(psd_addr)->_sin_family = (addr).sin_family; (psd_addr)->_sin_port = (addr).sin_port; (psd_addr)->_sin_addr = (addr).sin_addr.s_addr;}

#if 0 //defined(LINUX)
static struct ifreq ifr;
static BOOL g_ifr_got = FALSE;
#endif
_int32 sd_create_socket(_int32 domain, _int32 type, _int32 protocol, _u32 *sock)
{
	_int32 ret_val = SUCCESS;
	_int32 flags = 0;
    

#ifdef LINUX

	flags = 0;

   if(is_available_ci(CI_SOCKET_IDX_CREATE))
   {
        ret_val = ((et_socket_create)ci_ptr(CI_SOCKET_IDX_CREATE))(domain, type, protocol, sock);
    return ret_val;
   }

	*sock = socket(domain, type, protocol);

	if(*sock == (_u32)-1)
		return errno;

	if(*sock == 0)
	{/* find a case that socket descripor == 0 ... */

		/* try to get a non-zero descriptor */
		*sock = socket(domain, type, protocol);

		/* close the zero-descriptor */
		sd_close_socket(0);

		if(*sock == (_u32)-1)
			return errno;

		if(*sock == 0)
		{/* would this case happend? */
			return INVALID_SOCKET_DESCRIPTOR;
		}
	}

	/* set nonblock */
    flags = fcntl(*sock, F_GETFL);

	ret_val = fcntl(*sock, F_SETFL, flags | O_NONBLOCK);

	if(ret_val < 0)
	{
		LOG_DEBUG("set non-block failed:%d, so close the new socket(id:%d)", errno, *sock);

		sd_close_socket(*sock);
		*sock = INVALID_SOCKET;

		ret_val = errno;
	}
	else
	{
		ret_val = SUCCESS;

        if(is_available_ci(CI_SOCKET_IDX_SET_SOCKOPT))
            ret_val = ((et_socket_set_sockopt)ci_ptr(CI_SOCKET_IDX_SET_SOCKOPT))(*sock, type);

        if(ret_val != SUCCESS)
        {
            LOG_DEBUG("set sockopt failed:%d, so close the new socket(id:%d)", ret_val, *sock);

            sd_close_socket(*sock);
            *sock = INVALID_SOCKET;
        }

        LOG_DEBUG("success to create socket(id:%d)", *sock);
	}
#elif defined(WINCE)
	
	_u32 arg = 1;
	
	flags = 0;

   if(is_available_ci(CI_SOCKET_IDX_CREATE))
   {
        ret_val = ((et_socket_create)ci_ptr(CI_SOCKET_IDX_CREATE))(domain, type, protocol, sock);
    return ret_val;
   }
	LOG_DEBUG("sd_create_socket:domain=%d,type=%d,protocol=%d,SD_SOCK_DGRAM=%d,SD_SOCK_STREAM=%d", domain,type,protocol,SD_SOCK_DGRAM,SD_SOCK_STREAM);
	if((type ==SD_SOCK_DGRAM)&&(protocol==0))
		protocol = IPPROTO_UDP;
	else
	if((type ==SD_SOCK_STREAM)&&(protocol==0))
		protocol = IPPROTO_TCP;
		
	*sock = socket(domain, type, protocol);
	
	if(*sock == (_u32)-1)
		return GetLastError();
	
	/* set nonblock */
 	ret_val = ioctlsocket(*sock ,FIONBIO,&arg);
	
	if(ret_val < 0)
		ret_val = GetLastError();
#endif
	return ret_val;
}

_int32 sd_socket_bind(_u32 socket, SD_SOCKADDR *paddr)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)
	struct sockaddr_in os_addr;
	
     _int32 reuse = 1;
	_int32 socket_type = 0;
	socklen_t sock_type_len = sizeof(socket_type);

      sd_memset((void*) &os_addr, 0, sizeof(os_addr));

	CONV_FROM_SD_SOCKADDR(os_addr, paddr);

	if(getsockopt(socket, SOL_SOCKET, SO_TYPE, &socket_type, &sock_type_len)==0)
	{
		if(socket_type==SD_SOCK_STREAM)
		{
			setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
		}
	}

	ret_val = bind(socket, (const struct sockaddr*)&os_addr, sizeof(os_addr));
	if(ret_val < 0)
		ret_val = errno;

#elif defined(WINCE)
	struct sockaddr_in os_addr;
       _int32 reuse = 1;

	/* If the port is specified as 0, the service provider assigns a unique port to the application with a  appropriate value */
	paddr->_sin_port = 0;
	
	CONV_FROM_SD_SOCKADDR(os_addr, paddr);

	
       if(setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
       {
             //  setsockopt error 
       }

	ret_val = bind(socket, (const struct sockaddr*)&os_addr, sizeof(os_addr));
	if(ret_val < 0)
		ret_val = GetLastError();

	if(ret_val == 0)
	{
		SOCKADDR_IN addr = {0};
		int length = sizeof(addr);
		_u16 port = 0;
		getsockname(socket, (struct sockaddr*)&addr, &length);
		port = ntohs(addr.sin_port);
		if(addr.sin_port!=os_addr.sin_port)
		{
			paddr->_sin_port = addr.sin_port;
		}
	}
#endif
	return ret_val;
}

_int32 sd_socket_listen(_u32 socket, _int32 backlog)
{
	_int32 ret_val = SUCCESS;
#if defined(LINUX)

	ret_val = listen(socket, backlog);
	if(ret_val < 0)
		ret_val = errno;

#elif defined(WINCE)

	ret_val = listen(socket, backlog);
	if(ret_val < 0)
		ret_val = GetLastError();

#endif
	return ret_val;
}

_int32 sd_close_socket(_u32 socket)
{
	_int32 ret_val = SUCCESS;
	_int32 ret_err = SUCCESS;

       if(is_available_ci(CI_SOCKET_IDX_CLOSE))
       {
            ret_val = ((et_socket_close)ci_ptr(CI_SOCKET_IDX_CLOSE))(socket);
	    return ret_val;
       }
#if defined(LINUX)

#ifdef _ANDROID_LINUX
	shutdown(socket, SHUT_RDWR);
#endif

	do
	{
		ret_val = close(socket);
		ret_err = errno;
		if( ret_val < 0 )
		{
			LOG_URGENT("ERR to close socket(id:%d), err:%d", socket, ret_err);	
		}
	}while(ret_val < 0 && errno == EINTR);

	//close(socket);
	LOG_DEBUG("success to close socket(id:%d)", socket);
#elif defined(WINCE)
	closesocket(socket);
#endif

	return ret_val;
}

_int32 sd_accept(_u32 socket, _u32 *accept_sock, SD_SOCKADDR *addr)
{
	_int32 ret_val = SUCCESS;
#if defined(LINUX)	
	struct sockaddr_in os_addr;
	_u32 addr_len = sizeof(os_addr);
	_int32 flags = 0;

    sd_memset((void*) &os_addr, 0, sizeof(os_addr));

	while(1)
	{
		ret_val = accept(socket, (struct sockaddr*)&os_addr, &addr_len);
		if(ret_val < 0 && errno == EINTR)
			continue;

		break;
	}

	if(ret_val >= 0)
	{
		*accept_sock = ret_val;
		CONV_TO_SD_SOCKADDR(addr, os_addr);

		/* set socket nonblock */
		flags = fcntl(*accept_sock, F_GETFL);
		ret_val = fcntl(*accept_sock, F_SETFL, flags | O_NONBLOCK);

		if(ret_val < 0)
		{
			LOG_DEBUG("set non-block failed:%d, so close the accepted socket(id:%d)", errno, *accept_sock);

			/* close this socket? */
			sd_close_socket(*accept_sock);
			*accept_sock = INVALID_SOCKET;

			ret_val = errno;
		}
		else
		{
			ret_val = SUCCESS;

            if(is_available_ci(CI_SOCKET_IDX_SET_SOCKOPT))
                ret_val = ((et_socket_set_sockopt)ci_ptr(CI_SOCKET_IDX_SET_SOCKOPT))(*accept_sock, SD_SOCK_STREAM);

            if(ret_val != SUCCESS)
            {
                LOG_DEBUG("set sockopt failed:%d, so close the accpeted socket(id:%d)", ret_val, *accept_sock);

                /* close this socket? */
                sd_close_socket(*accept_sock);
                *accept_sock = INVALID_SOCKET;
            }

            LOG_DEBUG("success to accept new socket(id:%d)", *accept_sock);
		}
	}
	else
	{/* failed */
		ret_val = errno;
		if(ret_val == EAGAIN)
			ret_val = WOULDBLOCK;
	}
#elif defined(WINCE)
	struct sockaddr_in os_addr;
	_u32 addr_len = sizeof(os_addr);
	_int32 flags = 0;
	_u32   arg = 1;
	
	
	while(1)
	{
		ret_val = accept(socket, (struct sockaddr*)&os_addr, &addr_len);
		if(ret_val < 0 && WSAGetLastError() == WSAEINTR)
			continue;
		
		break;
	}
	
	if(ret_val >= 0)
	{
		*accept_sock = ret_val;
		CONV_TO_SD_SOCKADDR(addr, os_addr);
		
		/* set socket nonblock */
		/* set socket nonblock */
		ret_val = ioctlsocket(*accept_sock ,FIONBIO,&arg);
		
		if(ret_val < 0)
		{
			/* close this socket? */
			sd_close_socket(*accept_sock);
			
			ret_val = WSAGetLastError() ;
		}
		else
			ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = WSAGetLastError() ;
		if(ret_val == WSAEWOULDBLOCK)
			ret_val = WOULDBLOCK;
	}
#endif

	return ret_val;
}

_int32 sd_connect(_u32 socket, const SD_SOCKADDR *paddr)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)	
	struct sockaddr_in os_addr;
	_int32 addr_len = sizeof(os_addr);

#ifdef _DEBUG
        char ip_addr[16];
        sd_inet_ntoa(paddr->_sin_addr,  ip_addr, sizeof(ip_addr));
        ip_addr[15] = 0;
        LOG_ERROR("ready to connect (0x%X=)%s : (%u=)%d about (socket:%d)\n",paddr->_sin_addr, ip_addr, paddr->_sin_port,sd_ntohs(paddr->_sin_port), socket);
#endif

    sd_memset((void*) &os_addr, 0, sizeof(os_addr));

	CONV_FROM_SD_SOCKADDR(os_addr, paddr);

	do
	{
		ret_val = connect(socket, (struct sockaddr*)&os_addr, addr_len);
	}while(ret_val < 0 && errno == EINTR);

	if(ret_val >= 0)
	{
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
		if(ret_val == EINPROGRESS)
			ret_val = WOULDBLOCK;
	}

#elif defined(WINCE)
	struct sockaddr_in os_addr;
	_int32 addr_len = sizeof(os_addr);
	_int32 last_err = 0;
	
#ifdef _DEBUG
        char ip_addr[16];
        sd_inet_ntoa(paddr->_sin_addr,  ip_addr, sizeof(ip_addr));
        ip_addr[15] = 0;
        LOG_DEBUG("ready to connect %s : %d about (socket:%d)\n", ip_addr, sd_ntohs(paddr->_sin_port), socket);
#endif
	

	CONV_FROM_SD_SOCKADDR(os_addr, paddr);
	
	do
	{
		ret_val = connect(socket, (struct sockaddr*)&os_addr, addr_len);
		if(ret_val<0)
		{
			last_err = WSAGetLastError();
		}
	}while(ret_val < 0 && last_err== WSAEINTR);
	
	//LOG_DEBUG("sd_connect 1(fd:%u),ret_val=%d,last_err=%d ",socket,ret_val,last_err);
	if(ret_val >= 0)
	{
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = last_err;//WSAGetLastError();
		if(ret_val == WSAEWOULDBLOCK)
			ret_val = WOULDBLOCK;
	}
	
	//LOG_DEBUG("sd_connect 2(fd:%u),ret_val=%d ",socket,ret_val);
	
#endif

	return ret_val;
}

_int32 sd_asyn_proxy_connect(_u32 socket, const char* host, _u16 port,  void* user_own_data, void* user_msg_data)
{
	_int32 ret_val = SUCCESS;
	SD_SOCKADDR addr;
	SD_SOCKADDR *paddr=&addr;
	struct sockaddr_in os_addr;
	_int32 addr_len = sizeof(os_addr);
	_u32 proxy_ip = 0;
	_u16 proxy_port = 0;
#ifdef _DEBUG
     char ip_addr[16];
#endif
	ret_val = sd_get_proxy(&proxy_ip,&proxy_port);
	CHECK_VALUE(ret_val);
    	sd_memset((void*) &addr, 0, sizeof(SD_SOCKADDR));
	addr._sin_addr = proxy_ip;//0xAC00000A;//10.0.0.172
	addr._sin_port = sd_htons(proxy_port);
	addr._sin_family = SD_AF_INET;
#ifdef _DEBUG
        sd_inet_ntoa(paddr->_sin_addr,  ip_addr, sizeof(ip_addr));
        ip_addr[15] = 0;
        LOG_ERROR("ready to proxy connect (0x%X=)%s : (%u=)%d about (socket:%d)\n",paddr->_sin_addr, ip_addr, paddr->_sin_port,sd_ntohs(paddr->_sin_port), socket);
#endif

    sd_memset((void*) &os_addr, 0, sizeof(os_addr));

	CONV_FROM_SD_SOCKADDR(os_addr, paddr);

#if defined( WINCE)
	do
	{
		ret_val = connect(socket, (struct sockaddr*)&os_addr, addr_len);
	}while(ret_val < 0 && WSAGetLastError() == WSAEINTR);
	
	if(ret_val >= 0)
	{
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = WSAGetLastError();
		if(ret_val == WSAEWOULDBLOCK)
			ret_val = WOULDBLOCK;
	}
#else
	do
	{
		ret_val = connect(socket, (struct sockaddr*)&os_addr, addr_len);
	}while(ret_val < 0 && errno == EINTR);

	if(ret_val >= 0)
	{
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
		if(ret_val == EINPROGRESS)
			ret_val = WOULDBLOCK;
	}
#endif

	return ret_val;
}

_int32 sd_recv(_u32 socket, char *buffer, _int32 bufsize, _int32 *recved_len)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)	

	(*recved_len) = 0;

	do
	{
		ret_val = recv(socket, buffer, bufsize, 0);
	}while(ret_val < 0 && errno == EINTR);

	if(ret_val >= 0)
	{
		*recved_len = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
		if(ret_val == EAGAIN)
			ret_val = WOULDBLOCK;
	}
#elif defined(WINCE)
	(*recved_len) = 0;
	
	do
	{
		ret_val = recv(socket, buffer, bufsize, 0);
	}while(ret_val < 0 && WSAGetLastError() == WSAEINTR);
	
	if(ret_val >= 0)
	{
		*recved_len = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = WSAGetLastError();
		if(ret_val == WSAEWOULDBLOCK)
			ret_val = WOULDBLOCK;
	}
	
#endif

	return ret_val;
}

_int32 sd_recvfrom(_u32 socket, char *buffer, _int32 bufsize, SD_SOCKADDR *addr, _int32 *recved_len)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)	

	struct sockaddr_in os_addr;
	_u32 addr_len = sizeof(os_addr);

    sd_memset((void*) &os_addr, 0, sizeof(os_addr));

	*recved_len = 0;

	do
	{
		ret_val = recvfrom(socket, buffer, bufsize, 0, (struct sockaddr*)&os_addr, &addr_len);
	}while(ret_val < 0 && errno == EINTR);

	if(ret_val >= 0)
	{
		*recved_len = ret_val;
		CONV_TO_SD_SOCKADDR(addr, os_addr);
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
		if(ret_val == EAGAIN)
			ret_val = WOULDBLOCK;
	}

#elif defined(WINCE)
	struct sockaddr_in os_addr;
	_u32 addr_len = sizeof(os_addr);
	
	*recved_len = 0;
	
	do
	{
		ret_val = recvfrom(socket, buffer, bufsize, 0, (struct sockaddr*)&os_addr, &addr_len);
	}while(ret_val < 0 && WSAGetLastError() == WSAEINTR);
	
	if(ret_val >= 0)
	{
		*recved_len = ret_val;
		CONV_TO_SD_SOCKADDR(addr, os_addr);
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val =  WSAGetLastError();
		if(ret_val == WSAEWOULDBLOCK)
			ret_val = WOULDBLOCK;
	}
#endif

	return ret_val;
}

_int32 sd_send(_u32 socket, char *buffer, _int32 sendsize, _int32 *sent_len)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)	

	*sent_len = 0;

	do
	{
		ret_val = send(socket, buffer, sendsize, 0);
	}while(ret_val < 0 && errno == EINTR);

	if(ret_val >= 0)
	{
		*sent_len = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
		if(ret_val == EAGAIN)
			ret_val = WOULDBLOCK;
	}

#elif defined(WINCE)
	*sent_len = 0;
	
	do
	{
		ret_val = send(socket, buffer, sendsize, 0);
	}while(ret_val < 0 && WSAGetLastError() == WSAEINTR);
	
	if(ret_val >= 0)
	{
		*sent_len = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = WSAGetLastError() ;
		if(ret_val == WSAEWOULDBLOCK)
			ret_val = WOULDBLOCK;
	}

#endif

	return ret_val;
}

_int32 sd_sendto(_u32 socket, char *buffer, _int32 sendsize, const SD_SOCKADDR *addr, _int32 *sent_len)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)	

	struct sockaddr_in os_addr;
	_int32 addr_len = sizeof(os_addr);

    sd_memset((void*) &os_addr, 0, sizeof(os_addr));

	*sent_len = 0;
	CONV_FROM_SD_SOCKADDR(os_addr, addr);

	do
	{
		ret_val = sendto(socket, buffer, sendsize, 0, (const struct sockaddr*)&os_addr, addr_len);
	}while(ret_val < 0 && errno == EINTR);

	if(ret_val >= 0)
	{
		*sent_len = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = errno;
		if(ret_val == EAGAIN)
			ret_val = WOULDBLOCK;
	}

#elif defined(WINCE)

	struct sockaddr_in os_addr;
	_int32 addr_len = sizeof(os_addr);

	*sent_len = 0;
	CONV_FROM_SD_SOCKADDR(os_addr, addr);

	do
	{
		ret_val = sendto(socket, buffer, sendsize, 0, (const struct sockaddr*)&os_addr, addr_len);
	}while(ret_val < 0 && WSAGetLastError() == WSAEINTR);

	if(ret_val >= 0)
	{
		*sent_len = ret_val;
		ret_val = SUCCESS;
	}
	else
	{/* failed */
		ret_val = WSAGetLastError();
		if(ret_val == WSAEWOULDBLOCK)
			ret_val = WOULDBLOCK;
	}
#endif

	return ret_val;
}

_int32 sd_getpeername(_u32 socket, SD_SOCKADDR *addr)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)
	struct sockaddr_in os_addr;
	_u32 addr_len = sizeof(os_addr);

    sd_memset((void*) &os_addr, 0, sizeof(os_addr));

    ret_val = getpeername(socket, (struct sockaddr*)&os_addr, &addr_len);

	if(ret_val >= 0)
	{
		CONV_TO_SD_SOCKADDR(addr, os_addr);
		ret_val = SUCCESS;
	}
	else
	{
		ret_val = errno;
	}
#elif defined(WINCE)
	struct sockaddr_in os_addr;
	_u32 addr_len = sizeof(os_addr);

	ret_val = getpeername(socket, (struct sockaddr*)&os_addr, &addr_len);

	if(ret_val >= 0)
	{
		CONV_TO_SD_SOCKADDR(addr, os_addr);
		ret_val = SUCCESS;
	}
	else
	{
		ret_val = GetLastError();
	}

#endif

	return ret_val;
}

_int32 sd_getsockname(_u32 socket, SD_SOCKADDR *addr)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)
	struct sockaddr_in os_addr;
	_u32 addr_len = sizeof(os_addr);

    sd_memset((void*) &os_addr, 0, sizeof(os_addr));

    ret_val = getsockname(socket, (struct sockaddr*)&os_addr, &addr_len);

	if(ret_val >= 0)
	{
		CONV_TO_SD_SOCKADDR(addr, os_addr);
		ret_val = SUCCESS;
	}
	else
	{
		ret_val = errno;
	}

#elif defined(WINCE)
	struct sockaddr_in os_addr;
	_u32 addr_len = sizeof(os_addr);

	ret_val = getsockname(socket, (struct sockaddr*)&os_addr, &addr_len);

	if(ret_val >= 0)
	{
		CONV_TO_SD_SOCKADDR(addr, os_addr);
		ret_val = SUCCESS;
	}
	else
	{
		ret_val = GetLastError();
	}

#endif

	return ret_val;
}

_int32 sd_set_snd_timeout(_u32 socket, _u32 timeout)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)
	struct timeval time;
	time.tv_sec = timeout / 1000;
	time.tv_usec = timeout % 1000 * 1000;

	ret_val = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &time, sizeof(time));
	if(ret_val < 0)
		ret_val = errno;

#elif defined(WINCE)
	struct timeval time;
	time.tv_sec = timeout / 1000;
	time.tv_usec = timeout % 1000 * 1000;

	ret_val = setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &time, sizeof(time));
	if(ret_val < 0)
		ret_val = GetLastError();
#endif

	return ret_val;
}

_int32 sd_set_rcv_timeout(_u32 socket, _u32 timeout)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)
	struct timeval time;
	time.tv_sec = timeout / 1000;
	time.tv_usec = timeout % 1000 * 1000;

	ret_val = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time));
	if(ret_val < 0)
		ret_val = errno;
#elif defined(WINCE)
	struct timeval time;
	time.tv_sec = timeout / 1000;
	time.tv_usec = timeout % 1000 * 1000;

	ret_val = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &time, sizeof(time));
	if(ret_val < 0)
		ret_val = GetLastError();

#endif

	return ret_val;
}


_int32 get_socket_error(_u32 sock)
{
	_int32 ret_val = SUCCESS;

#if defined(LINUX)
	_int32 errcode = 0;
	_u32 errcode_size = sizeof(_int32);
	ret_val = getsockopt(sock, SOL_SOCKET, SO_ERROR, &errcode, &errcode_size);
	if(ret_val < 0)
		ret_val = errno;
	else
		ret_val = errcode;
#elif defined(WINCE)
	_int32 errcode = 0;
	_u32 errcode_size = sizeof(_int32);
	ret_val = getsockopt(sock, SOL_SOCKET, SO_ERROR, &errcode, &errcode_size);
	if(ret_val < 0)
		ret_val = GetLastError();
	else
		ret_val = errcode;
#endif

	return ret_val;
}

_int32 sd_set_socket_ttl(_u32 sock, _int32 ttl)
{
	_int32 ret_val = SUCCESS;
#if defined(LINUX)
	ret_val = setsockopt(sock, IPPROTO_IP, IP_TTL, (void*)&ttl, sizeof(_int32));
	if(ret_val < 0)
		ret_val = errno;
	else
		ret_val = SUCCESS;
#else
	sd_assert(FALSE);
#endif
	return ret_val;
}

void* sd_gethostbyname(const char * name)
{
#if defined(WINCE)
        return (void*)gethostbyname(name);
#else
        return NULL;
#endif
}



