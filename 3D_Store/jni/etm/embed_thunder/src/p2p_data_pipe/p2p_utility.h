/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/08/14
-----------------------------------------------------------------------------------------------------------*/
#ifndef _P2P_UTILITY_H_
#define _P2P_UTILITY_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_data_pipe/p2p_data_pipe.h"
#include "utility/define.h"

_int32 get_p2p_capability(void);

BOOL is_p2p_pipe_connected(const P2P_DATA_PIPE* p2p_pipe);
#ifdef __cplusplus
}
#endif
#endif

