/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/11/3
-----------------------------------------------------------------------------------------------------------*/
#ifndef _UDT_INTERFACE_H_
#define _UDT_INTERFACE_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_transfer_layer/udt/udt_device.h"
#include "asyn_frame/msg.h"

_int32 init_udt_modular(void);

_int32 uninit_udt_modular(void);

_int32 udt_recv_buffer_comparator(void* E1, void* E2);

_int32 udt_device_create(UDT_DEVICE** udt, _u32 source_port, _u32 target_port, _u32 peerid_hashcode, PTL_DEVICE* device);

_int32 udt_device_init(UDT_DEVICE* udt, _u32 remote_ip, _u16 remote_port, _u32 seq, _u32 ack, _u32 wnd);

_int32 udt_device_close(UDT_DEVICE* udt);

_int32 udt_device_connect(UDT_DEVICE* udt, _u32 ip, _u16 udp_port);

_int32 udt_device_rebuild_package_and_send(UDT_DEVICE* udt, char* buffer, _u32 buffer_len, BOOL is_data);

_int32 udt_device_send(UDT_DEVICE* udt, char* buffer, _u32 len, BOOL is_data);

_int32 udt_device_recv(UDT_DEVICE* udt, RECV_TYPE type, char* buffer, _u32 expect_len, _u32 buffer_len);

_int32 udt_device_notify_send_data(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 udt_device_notify_recv_data(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 udt_device_notify_punch_hole_success(UDT_DEVICE* udt, _u32 remote_ip, _u16 remote_port);

#ifdef __cplusplus
}
#endif
#endif

