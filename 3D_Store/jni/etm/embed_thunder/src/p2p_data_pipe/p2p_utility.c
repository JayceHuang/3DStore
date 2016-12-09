#include "p2p_utility.h"
#include "utility/local_ip.h"

_int32 get_p2p_capability()
{			
	return 105;		//support_choke and support_resource_optimization and support_fin and support_extradata
}

BOOL is_p2p_pipe_connected(const P2P_DATA_PIPE* p2p_pipe)
{
	if(p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_CONNECTED || p2p_pipe->_data_pipe._dispatch_info._pipe_state == PIPE_DOWNLOADING)
		return TRUE;
	else
		return FALSE;
}

