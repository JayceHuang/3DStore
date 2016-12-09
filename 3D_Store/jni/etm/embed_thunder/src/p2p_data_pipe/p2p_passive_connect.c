#include "p2p_passive_connect.h"
#include "p2p_cmd_define.h"
#include "p2p_data_pipe.h"
#include "p2p_pipe_interface.h"
#include "p2p_memory_slab.h"
#include "p2p_socket_device.h"
#include "p2p_cmd_extractor.h"
#include "p2p_pipe_impl.h"
#include "p2p_transfer_layer/ptl_utility.h"
#include "p2p_transfer_layer/ptl_interface.h"
#include "p2p_transfer_layer/ptl_passive_connect.h"
#include "p2p_transfer_layer/tcp/tcp_memory_slab.h"
#include "p2p_transfer_layer/tcp/tcp_device.h"
#include "p2p_transfer_layer/udt/udt_device.h"
#include "task_manager/task_manager.h"
#include "connect_manager/connect_manager_interface.h"
#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif
#include "socket_proxy_interface.h"
#include "asyn_frame/device.h"
#include "utility/map.h"
#include "utility/utility.h"
#include "utility/mempool.h"
#include "utility/logid.h"
#define  LOGID	LOGID_P2P_PIPE
#include "utility/logger.h"

//接收到对方的被动连接
_int32 p2p_handle_passive_accept(PTL_DEVICE** device, char* buffer, _u32 len)
{
	_int32 ret = SUCCESS;
	P2P_DATA_PIPE* p2p_pipe = NULL;
	HANDSHAKE_CMD cmd;
	LOG_DEBUG("[device = 0x%x]p2p_handle_passive_accept.", *device);
#ifdef UPLOAD_ENABLE
	if(ulm_is_pipe_full() == TRUE)
	{
		LOG_DEBUG("[device = 0x%x]p2p_handle_passive_accept, but upload pipe is full.");
		return SUCCESS;
	}
#endif
	ret = extract_handshake_cmd(buffer, len, &cmd);
	if(ret != SUCCESS)
	{
		LOG_ERROR("p2p_handle_passive_accept, but extract_handshake_cmd failed, ret = %d.", ret);
		return ret;
    } 
	
	LOG_DEBUG("p2p_handle_passive_accept, recv handshake cmd, remote_version = %u, remote_peerid = %s, product_id = %u.", cmd._header._version, cmd._peerid, cmd._product_ver);
#ifdef UPLOAD_ENABLE
	//注意一定要先查看是否纯上传，再查看是否边下边传，否则会导致纯上传的任务被认为边下边传
	//查看是否纯上传
	if(ulm_is_file_exist(cmd._gcid, cmd._file_size) == TRUE)
	{
		if(cmd._header._version < P2P_PROTOCOL_VERSION_58)
		{
			LOG_DEBUG("[device = 0x%x]this is only upload pipe, but not support choke/unchoke, not upload to this pipe, close it.", *device);
			return ret;
		}
		ret = p2p_pipe_create(NULL, NULL, (DATA_PIPE**)&p2p_pipe, *device);
		if(ret != SUCCESS)
			return ret;
		(*device) = NULL;		//已经被p2p_pipe接管
		ret = ulm_add_pipe_by_gcid(cmd._gcid, &p2p_pipe->_data_pipe);
		if(ret != SUCCESS)
		{
			LOG_ERROR("p2p_handle_passive_accept, but ulm_add_pipe_by_gcid failed, errcode = %d.", ret);
			return p2p_pipe_close(&p2p_pipe->_data_pipe);
		}	
		return p2p_handle_passive_connect(p2p_pipe, FALSE);
	}
#endif
	//查看是否边下边传
    if (tm_is_task_exist(cmd._gcid, TRUE) == TRUE)
	{	
		RESOURCE* resource = NULL;
		CONNECT_MANAGER* manager = NULL;
		_u32 file_index = 0;
		LOG_DEBUG("p2p_handle_passive_accept, task is exist, is download and upload pipe.");
		manager = tm_get_task_connect_manager(cmd._gcid,  &file_index, NULL);
		if(cm_is_pause_pipes(manager))
		{
			/* 暂停下载情况下不接收被动连接by zyq @20110117  */
			return ret;
		}
		
		if(cm_is_cdn_peer_res_exist_by_peerid(manager, cmd._peerid))
		{
			// 该peer是一个cdn资源，手雷主动链接它， 不接受其被动链接
			// 避免上传的速度不计算到cdn中
			LOG_DEBUG("p2p_handle_passive_accept, cm_is_cdn_peer_res_exist_by_peerid just return...");
			return ret;
		}
		
		ret = p2p_resource_create(&resource,   cmd._peerid, cmd._gcid, cmd._file_size, 0, 0, 0, 0, 0, P2P_FROM_PASSIVE,0 );
		CHECK_VALUE(ret);

		set_resource_max_retry_time(resource, 0);
		
        ret = p2p_pipe_create(NULL, NULL, (DATA_PIPE**)&p2p_pipe, *device);
		if(ret != SUCCESS)
		{
			p2p_resource_destroy(&resource);
			return ret;
		}

		(*device) = NULL;		//已经被p2p_pipe接管

		ret = cm_add_passive_peer(manager, file_index, resource, &p2p_pipe->_data_pipe);
		if(ret != SUCCESS)
		{
			LOG_DEBUG("p2p_handle_passive_accept, but cm_add_passive_peer failed, errcode = %d.", ret );
			p2p_pipe_close(&p2p_pipe->_data_pipe);
			return p2p_resource_destroy(&resource);
		}

		return p2p_handle_passive_connect(p2p_pipe, TRUE);
	}
	LOG_ERROR("accept a p2p connection, but is not broker connect or upload pipe, close it.");
	return ret;
}


