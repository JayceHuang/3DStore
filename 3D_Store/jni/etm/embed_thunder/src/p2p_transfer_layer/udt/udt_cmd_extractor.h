/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/11/13
-----------------------------------------------------------------------------------------------------------*/
#ifndef _UDT_CMD_EXTRACTOR_H_
#define _UDT_CMD_EXTRACTOR_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_transfer_layer/udt/udt_cmd_define.h"

struct tagPUNCH_HOLE_CMD;

_int32 udt_extract_data_transfer_cmd(char* buffer, _u32 len, DATA_TRANSFER_CMD* cmd);

_int32 udt_extract_reset_cmd(char* buffer, _u32 len, RESET_CMD* cmd);

_int32 udt_extract_syn_cmd(char* buffer, _u32 len, SYN_CMD* cmd);

_int32 udt_extract_keepalive_cmd(char* buffer, _u32 len, NAT_KEEPALIVE_CMD* cmd);

_int32 udt_extract_advanced_ack_cmd(char* buffer, _u32 len, ADVANCED_ACK_CMD* cmd);

_int32 udt_extract_advanced_data_cmd(char* buffer, _u32 len, ADVANCED_DATA_CMD* cmd);
#ifdef __cplusplus
}
#endif
#endif
