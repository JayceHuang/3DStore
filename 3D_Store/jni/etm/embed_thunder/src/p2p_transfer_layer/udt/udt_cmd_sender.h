/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/07/21
-----------------------------------------------------------------------------------------------------------*/
#ifndef _UDT_CMD_SENDER_H_
#define _UDT_CMD_SENDER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

_int32 udt_send_syn_cmd(BOOL is_syn_ack, _u32 seq, _u32 ack, _u32 wnd, _u32 source_port, _u32 target_port, _u32 ip, _u16 udp_port);
#ifdef __cplusplus
}
#endif
#endif



