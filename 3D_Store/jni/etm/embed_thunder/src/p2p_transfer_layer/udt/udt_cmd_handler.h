/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/11/13
-----------------------------------------------------------------------------------------------------------*/
#ifndef _UDT_CMD_HANDLER_H_
#define	_UDT_CMD_HANDLER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

struct tagSYN_CMD;

_int32 udt_handle_syn_cmd(struct tagSYN_CMD* cmd, _u32 remote_ip, _u16 remote_port);

_int32 udt_handle_advance_data_cmd(char** buffer, _u32 len);

_int32 udt_handle_advance_ack_cmd(char* buffer, _u32 len);

_int32 udt_handle_reset_cmd(char* buffer, _u32 len);

_int32 udt_handle_nat_keepalive_cmd(char* buffer, _u32 len);
#ifdef __cplusplus
}
#endif
#endif

