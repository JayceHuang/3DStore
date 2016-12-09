/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/12/3
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_UDT_IMPL_H_
#define _EMULE_UDT_IMPL_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_transfer_layer/emule_udt_device.h"
#include "asyn_frame/msg.h"

_int32 emule_udt_on_ping(EMULE_UDT_DEVICE* udt_device, _u32 conn_num);

_int32 emule_udt_on_keepalive(_u32 ip, _u16 port);

_int32 emule_udt_on_ack(EMULE_UDT_DEVICE* udt_device, _u32 ack_num, _u32 wnd_size);

_int32 emule_udt_on_reset(EMULE_UDT_DEVICE* udt_device);

_int32 emule_udt_on_nat_failed(EMULE_UDT_DEVICE* udt_device);

_int32 emule_udt_on_nat_reping(EMULE_UDT_DEVICE* udt_device);

_int32 emule_udt_on_nat_sync2(EMULE_UDT_DEVICE* udt_device, _u32 ip, _u16 port);

_int32 emule_udt_on_data(EMULE_UDT_DEVICE* udt_device, char** buffer, _int32 len);

_int32 emule_udt_on_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 emule_udt_read_data(EMULE_UDT_DEVICE* udt_device);

_int32 emule_udt_notify_read_msg(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

#endif
#endif /* ENABLE_EMULE */
#endif


