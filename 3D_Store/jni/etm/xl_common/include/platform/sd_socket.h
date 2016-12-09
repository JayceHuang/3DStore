#ifndef SD_SOCKET_H_00138F8F2E70_200806111927
#define SD_SOCKET_H_00138F8F2E70_200806111927
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

#if defined(LINUX)
#include <sys/types.h>
#include <sys/socket.h>
#elif defined(WINCE)
#define AF_INET 2
#define PF_INET AF_INET
#define SOCK_STREAM     1
#define SOCK_DGRAM		2
#endif

typedef _u32 SOCKET;
#define INVALID_SOCKET (0xFFFFFFFF)


typedef struct tagSD_SOCKADDR{
	_u16 _sin_family;
	_u16 _sin_port;
	_u32 _sin_addr;
} SD_SOCKADDR, *pSD_SOCKADDR;

typedef struct {
	_u8	 _ip_verlen;		/* ip version and ip header lenth*/  
	_u8	 _ip_tos;			/* ip type of service */  
	_u16 _ip_len;			/* ip packet lenghth */  
	_u16 _ip_id;			/* ip packet identification */  
	_u16 _ip_fragoff;		/* ip packet fragment and offset */  
	_u8  _ip_ttl;			/* ip packet time to live */  
	_u8	 _ip_proto;			/* ip packet protocol type */  
	_u16 _ip_chksum;		/* ip packet header checksum */  
	_u32 _ip_src_addr;		/* ip source ip adress */  
	_u32 _ip_dst_addr;		/* ip destination ip adress */  
} SD_IP_HEADER;

typedef struct {
	_u8	 _type;
  	_u8	 _code;
  	_u16 _checksum;
	_u16 _id;
    _u16 _sequence;
} SD_ICMP_HEADER;

#ifndef ICMP_ECHO
#define ICMP_ECHO		(8)
#define ICMP_ECHOREPLY	(0)
#endif
#define SD_IPPROTO_ICMP	(1)
#define WOULDBLOCK		(-2)

#define SD_PF_INET		(PF_INET)
#define SD_AF_INET		(SD_PF_INET)
#define SD_SOCK_DGRAM	(SOCK_DGRAM)
#define SD_SOCK_STREAM	(SOCK_STREAM)
#define	SD_SOCK_RAW		(SOCK_RAW)

#define ANY_ADDRESS		((_u32)0x00000000)

/* all socket are non-blocked */
_int32 sd_create_socket(_int32 domain, _int32 type, _int32 protocol, _u32 *socket);
_int32 sd_socket_bind(_u32 socket, SD_SOCKADDR *addr);
_int32 sd_socket_listen(_u32 socket, _int32 backlog);
_int32 sd_close_socket(_u32 socket);

/* need asyn operation */
_int32 sd_accept(_u32 socket, _u32 *accept_sock, SD_SOCKADDR *addr);

_int32 sd_connect(_u32 socket, const SD_SOCKADDR *addr);
_int32 sd_asyn_proxy_connect(_u32 socket, const char* host, _u16 port,  void* user_own_data, void* user_msg_data);

_int32 sd_recv(_u32 socket, char *buffer, _int32 bufsize, _int32 *recved_len);
_int32 sd_recvfrom(_u32 socket, char *buffer, _int32 bufsize, SD_SOCKADDR *addr, _int32 *recved_len);

_int32 sd_send(_u32 socket, char *buffer, _int32 sendsize, _int32 *sent_len);
_int32 sd_sendto(_u32 socket, char *buffer, _int32 sendsize, const SD_SOCKADDR *addr, _int32 *sent_len);


/* some api */

/* @Simple Function@
 * Return : errcode of sock.
 */
_int32 get_socket_error(_u32 sock);


_int32 sd_set_snd_timeout(_u32 socket, _u32 timeout /* ms */);
_int32 sd_set_rcv_timeout(_u32 socket, _u32 timeout /* ms */);


_int32 sd_getpeername(_u32 socket, SD_SOCKADDR *addr);

_int32 sd_getsockname(_u32 socket, SD_SOCKADDR *addr);

_int32 sd_set_socket_ttl(_u32 socket, _int32 ttl);
void* sd_gethostbyname(const char * name);

#ifdef __cplusplus
}
#endif
#endif
