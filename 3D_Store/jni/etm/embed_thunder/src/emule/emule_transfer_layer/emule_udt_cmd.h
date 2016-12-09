/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/12/3
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_UDT_CMD_H_
#define _EMULE_UDT_CMD_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_transfer_layer/emule_udt_device.h"

#define	DATA_SEGMENT_HEADER_SIZE	14

_int32 emule_udt_send_syn(EMULE_UDT_DEVICE* udt_device);

_int32 emule_udt_send_ping(EMULE_UDT_DEVICE* udt_device, BOOL keepalive);

_int32 emule_udt_send_data(EMULE_UDT_DEVICE* udt_device, EMULE_UDT_SEND_BUFFER* send_buffer);

_int32 emule_udt_send_ack(EMULE_UDT_DEVICE* udt_device, _u32 seq);

_int32 emule_udt_send_reping(EMULE_UDT_DEVICE* udt_device);

_int32 emule_recv_udt_cmd(char** buffer, _int32 len, _u32 ip, _u16 port);

_int32 emule_udt_recv_ping(char* buffer, _int32 len, _u32 ip, _u16 port);

_int32 emule_udt_recv_nat_register(char* buffer, _int32 len, _u32 ip, _u16 port);

_int32 emule_udt_recv_nat_failed(char* buffer, _int32 len, _u32 ip, _u16 port);

_int32 emule_udt_recv_nat_reping(char* buffer, _int32 len, _u32 ip, _u16 port);

_int32 emule_udt_recv_nat_sync2(char* buffer, _int32 len);

_int32 emule_udt_recv_data(char** buffer, _int32 len, _u32 ip, _u16 port);

_int32 emule_udt_recv_ack(char* buffer, _int32 len, _u32 ip, _u16 port);

_int32 emule_udt_recv_reset(char* buffer, _int32 len, _u32 ip, _u16 port);

#endif
#endif /* ENABLE_EMULE */
#endif


