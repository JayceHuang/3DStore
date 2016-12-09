/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/07/21
-----------------------------------------------------------------------------------------------------------*/
#ifndef _UDT_CMD_BUILDER_H_
#define _UDT_CMD_BUILDER_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

struct tagSYN_CMD;

_int32 udt_build_syn_cmd(char** buffer, _u32* len, struct tagSYN_CMD* cmd);


#ifdef __cplusplus
}
#endif
#endif



