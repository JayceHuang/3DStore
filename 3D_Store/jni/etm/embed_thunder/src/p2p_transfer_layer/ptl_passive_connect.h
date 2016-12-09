#ifndef _PTL_PASSIVE_CONNECT_H_
#define _PTL_PASSIVE_CONNECT_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_transfer_layer/ptl_interface.h"
#include "platform/sd_socket.h"

#define	PTL_PASSIVE_HEADER_LEN		9
#define	PTL_PASSIVE_BUFFER_LEN		256

typedef struct tagPTL_ACCEPT_DATA
{
	PTL_DEVICE*	_device;
	char		_buffer[PTL_PASSIVE_BUFFER_LEN];
	_u32 		_offset;
	_u32		_broker_seq;
}PTL_ACCEPT_DATA;

struct tagSYN_CMD;
struct tagUDP_BROKER_CMD;
struct tagPASSIVE_TCP_BROKER_DATA;
struct tagPASSIVE_UDP_BROKER_DATA;


typedef _int32 (*p2p_accept_func)(PTL_DEVICE** device, char* buffer, _u32 len);

_int32 ptl_accept_data_comparator(void* E1, void* E2);

void ptl_regiest_p2p_accept_func(p2p_accept_func func);

_int32 ptl_create_passive_connect(void);

_int32 ptl_close_passive_connect(void);

_int32 force_close_p2p_acceptor_socket(void);

_int32 ptl_erase_ptl_accept_data(PTL_ACCEPT_DATA* data);

_int32 ptl_accept_passive_tcp_connect(_int32 errcode, _u32 pending_op_count, SOCKET conn_sock, void* user_data);

_int32 ptl_accept_passive_tcp_broker_connect(struct tagPASSIVE_TCP_BROKER_DATA* broker_data);

_int32 ptl_accept_passive_udp_broker_connect(struct tagPASSIVE_UDP_BROKER_DATA* broker_data);

_int32 ptl_accept_udt_passvie_connect(struct tagSYN_CMD* cmd,  _u32 remote_ip, _u16 remote_port);

_int32 ptl_passive_connect_callback(_int32 errcode, PTL_DEVICE* device);

_int32 ptl_passive_recv_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_recv);

_int32 ptl_passive_send_callback(_int32 errcode, PTL_DEVICE* device, _u32 had_send);

_int32 ptl_passive_close_callback(PTL_DEVICE* device);
#ifdef __cplusplus
}
#endif
#endif

