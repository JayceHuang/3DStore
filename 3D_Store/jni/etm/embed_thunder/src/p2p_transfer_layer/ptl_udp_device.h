/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/10/9
-----------------------------------------------------------------------------------------------------------*/
#ifndef _PTL_UDP_DEVICE_H_
#define	_PTL_UDP_DEVICE_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#include "platform/sd_socket.h"

_int32 ptl_create_udp_device(void);

_int32 ptl_close_udp_device(void);

_int32 ptl_force_close_udp_device_socket(void);

SOCKET ptl_get_global_udp_socket(void);

/*调用该函数后，则buffer被该函数接管；如果发送失败的话，该函数会主动free掉buffer*/
_int32 ptl_udp_sendto(const char* buffer, _u32 len, _u32 ip, _u16 port);

_int32 ptl_udp_sendto_by_domain(const char* buffer, _u32 len, const char* host, _u32 port);

_int32 ptl_udp_sendto_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);

_int32 ptl_udp_recvfrom(void);

_int32 ptl_udp_recvfrom_callback(_int32 errcode, _u32 pending_op_count, char* buffer, _u32 had_recv, SD_SOCKADDR* from, void* user_data);
#ifdef __cplusplus
}
#endif
#endif

