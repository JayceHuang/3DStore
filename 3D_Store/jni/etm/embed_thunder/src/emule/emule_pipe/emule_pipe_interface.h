/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/10/13
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_PIPE_INTERFACE_H_
#define _EMULE_PIPE_INTERFACE_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "emule/emule_utility/emule_peer.h"
#include "connect_manager/data_pipe.h"

struct tagRESOURCE;
struct tagEMULE_PIPE_DEVICE;
struct tagEMULE_UPLOAD_DEVICE;
struct _tag_range_data_buffer;

typedef struct tagEMULE_DATA_PIPE
{
	DATA_PIPE						_data_pipe;
	struct tagEMULE_PIPE_DEVICE*	_device;
	struct tagEMULE_UPLOAD_DEVICE*	_upload_device;
	EMULE_PEER						_remote_peer;
	_u32							_rank;
	_u32							_timeout_id;
	_u32							_last_ack_time;
	_u32							_random;
}EMULE_DATA_PIPE;

_int32 emule_pipe_create(DATA_PIPE** pipe, void* data_manager, struct tagRESOURCE* res);

_int32 emule_pipe_open(DATA_PIPE* pipe);

_int32 emule_pipe_close(DATA_PIPE* pipe);

_int32 emule_pipe_change_ranges(DATA_PIPE* pipe, const RANGE* range, BOOL cancel_flag);

_int32 emule_pipe_get_speed(DATA_PIPE* pipe);

#ifdef UPLOAD_ENABLE
_int32 emule_pipe_choke_remote(DATA_PIPE* pipe, BOOL is_choke);
#endif

#endif
#endif /* ENABLE_EMULE */
#endif


