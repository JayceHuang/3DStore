
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule_pipe_interface.h"
#include "emule_pipe_device.h"
#include "emule_pipe_impl.h"
#include "emule_pipe_upload.h"
#include "emule/emule_interface.h"
#include "emule/emule_impl.h"
#include "./emule_pipe_wait_accept.h"
#include "utility/mempool.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define	 LOGID	LOGID_EMULE
#include "utility/logger.h"
#include "emule_pipe_cmd.h"

#ifdef UPLOAD_ENABLE
#include "upload_manager/upload_manager.h"
#endif

_int32 emule_pipe_create( DATA_PIPE** pipe, void* data_manager, struct tagRESOURCE* res)
{
	EMULE_DATA_PIPE* emule_pipe = NULL;
	_int32 ret = SUCCESS;
	ret = sd_malloc(sizeof(EMULE_DATA_PIPE), (void**)&emule_pipe);
	if(ret != SUCCESS)
		return ret;
	sd_memset(emule_pipe, 0, sizeof(EMULE_DATA_PIPE));
	emule_pipe->_rank = MAX_U32;
	emule_pipe->_timeout_id = INVALID_MSG_ID;
	init_pipe_info(&emule_pipe->_data_pipe, EMULE_PIPE, res, data_manager);
	emule_peer_init(&emule_pipe->_remote_peer);
	LOG_DEBUG("[emule_pipe = 0x%x, res = 0x%x]emule_pipe_create.", emule_pipe, res);
	*pipe = (DATA_PIPE*)emule_pipe;
	return ret;
}

_int32 emule_pipe_open(DATA_PIPE* pipe)
{
	_int32 ret = SUCCESS;
	EMULE_PIPE_DEVICE* pipe_device = NULL;
	EMULE_DATA_PIPE* emule_pipe = (EMULE_DATA_PIPE*)pipe;
	EMULE_RESOURCE* emule_res = (EMULE_RESOURCE*)emule_pipe->_data_pipe._p_resource;
#ifdef _DEBUG
	char buffer[24] = {0};
	char user_id_buffer[33] = {0};
	str2hex((char*)emule_res->_user_id, USER_ID_SIZE, user_id_buffer, 33);
	sd_inet_ntoa(emule_res->_client_id, buffer, 24);
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_open, connect to ip = %s, port = %u, user_id = %s."
        , emule_pipe, buffer, emule_res->_client_port, user_id_buffer);
#endif
	ret = emule_pipe_device_create(&pipe_device);
	CHECK_VALUE(ret);
	emule_pipe_attach_emule_device(emule_pipe, pipe_device);
	ret = emule_pipe_device_connect(emule_pipe->_device, emule_res);
	if(ret != SUCCESS)
		return ret;
	emule_pipe_change_state(emule_pipe, PIPE_CONNECTING);
	return ret;
}


_int32 emule_pipe_close(struct tagDATA_PIPE* pipe)
{
	EMULE_DATA_PIPE* emule_pipe = (EMULE_DATA_PIPE*)pipe;
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_close, pipe_state = %d.", emule_pipe, pipe->_dispatch_info._pipe_state);
	if(emule_pipe->_timeout_id != INVALID_MSG_ID)
		cancel_timer(emule_pipe->_timeout_id);
    
#ifdef UPLOAD_ENABLE
	ulm_cancel_read_data(pipe);
    ulm_remove_pipe(pipe);
#endif

    emule_remove_wait_accept_pipe(emule_pipe);
    dp_set_pipe_interface(pipe, NULL);
	emule_upload_device_close(&emule_pipe->_upload_device);
	emule_download_queue_remove(emule_pipe);
	if(emule_pipe->_device != NULL)
		emule_pipe_device_close(emule_pipe->_device);
	uninit_pipe_info(&emule_pipe->_data_pipe);
	emule_peer_uninit(&emule_pipe->_remote_peer);
	return sd_free(emule_pipe);
}

_int32 emule_pipe_change_ranges(DATA_PIPE* pipe, const RANGE* range, BOOL cancel_flag)
{
	EMULE_DATA_PIPE* emule_pipe = (EMULE_DATA_PIPE*)pipe;
	if(emule_pipe->_data_pipe._dispatch_info._pipe_state != PIPE_DOWNLOADING)
	{
		LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_change_ranges, but state is  not DOWNLOADING state, just return.", emule_pipe);
		return SUCCESS;
	}
	dp_clear_uncomplete_ranges_list(&emule_pipe->_data_pipe);
	dp_add_uncomplete_ranges(&emule_pipe->_data_pipe, range);
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_change_ranges, uncomplete_range[%u, %u],down_range[%u, %u],cancel_flag = %d", 
				emule_pipe, range->_index, range->_num, emule_pipe->_data_pipe._dispatch_info._down_range._index, 
				emule_pipe->_data_pipe._dispatch_info._down_range._num, cancel_flag);
	if(cancel_flag == TRUE)
		LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_change_ranges, cancel_flag is TRUE, but emule pipe can't cancel.", emule_pipe);
	return emule_pipe_request_data(emule_pipe);
}

_int32 emule_pipe_get_speed(DATA_PIPE* pipe)
{
	_int32 ret = SUCCESS;
	EMULE_DATA_PIPE* emule_pipe = (EMULE_DATA_PIPE*)pipe;
	sd_assert(pipe->_data_pipe_type == EMULE_PIPE && pipe->_dispatch_info._pipe_state == PIPE_DOWNLOADING);
	ret = calculate_speed(&emule_pipe->_device->_download_speed, &pipe->_dispatch_info._speed);
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_get_speed, speed = %u.", emule_pipe, pipe->_dispatch_info._speed);
	return ret;
}

#ifdef UPLOAD_ENABLE
_int32 emule_pipe_choke_remote(DATA_PIPE* pipe, BOOL is_choke)
{
	_int32 ret = SUCCESS;
	EMULE_DATA_PIPE* emule_pipe = (EMULE_DATA_PIPE*)pipe;
    
	LOG_DEBUG("[emule_pipe = 0x%x]emule_pipe_choke_remote, is_choke = %d.", emule_pipe, is_choke);
    if(is_choke)
    {
        emule_upload_device_close(&emule_pipe->_upload_device);
        ulm_cancel_read_data(pipe);
        pipe->_dispatch_info._pipe_upload_state = UPLOAD_CHOKE_STATE;
        emule_pipe_send_accept_upload_req_cmd((EMULE_DATA_PIPE*)pipe, FALSE);
    }
    else
    {
    	if(emule_pipe->_upload_device == NULL);
		{
			ret = emule_upload_device_create(&emule_pipe->_upload_device, emule_pipe);
			if(ret != SUCCESS) return ret;
		}
        pipe->_dispatch_info._pipe_upload_state = UPLOAD_UNCHOKE_STATE;
        emule_pipe_send_accept_upload_req_cmd((EMULE_DATA_PIPE*)pipe, TRUE);
    }
    return SUCCESS;
}
#endif
#endif /* ENABLE_EMULE */
#endif /* ENABLE_EMULE */

