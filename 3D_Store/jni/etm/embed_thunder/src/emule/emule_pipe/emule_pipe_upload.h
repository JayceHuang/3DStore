/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/18
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_PIPE_UPLOAD_H_
#define _EMULE_PIPE_UPLOAD_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_pipe/emule_pipe_interface.h"

struct tagTMP_FILE;
struct _tag_range_data_buffer;

typedef struct tagEMULE_UPLOAD_REQ
{
	_u64	_data_pos;
	_u32	_data_len;
	char*	_buffer;
	_u32	_buffer_offset;
	_u32	_buffer_len;
}EMULE_UPLOAD_REQ;

typedef struct tagEMULE_UPLOAD_DEVICE
{
	LIST							_upload_req_list;
	EMULE_UPLOAD_REQ*			_cur_upload;
	struct _tag_range_data_buffer*	_upload_data_buffer;
	struct tagEMULE_DATA_PIPE*	_emule_pipe;
	BOOL						_is_own_cur_upload_buffer;
	BOOL						_close_flag;
    BOOL                        _reading_flag;
}EMULE_UPLOAD_DEVICE;

_int32 emule_upload_device_create(EMULE_UPLOAD_DEVICE** upload_device, EMULE_DATA_PIPE* emule_pipe);

_int32 emule_upload_device_close(EMULE_UPLOAD_DEVICE** upload_device);

_int32 emule_upload_recv_request(EMULE_UPLOAD_DEVICE* upload_device, _u64 start_pos1, _u64 end_pos1, _u64 start_pos2, _u64 end_pos2, _u64 start_pos3, _u64 end_pos3);

_int32 emule_pipe_add_upload_req(EMULE_UPLOAD_DEVICE* upload_device, _u64 start_pos, _u64 end_pos);

_int32 emule_upload_process_call_back(void* user);

BOOL emule_pipe_is_upload_req_exist(EMULE_UPLOAD_DEVICE* upload_device, EMULE_UPLOAD_REQ* req);

_int32 emule_upload_process_request(EMULE_UPLOAD_DEVICE* upload_device);

_int32 emule_upload_read_data(EMULE_UPLOAD_DEVICE* upload_device);

_int32 emule_pipe_device_try_send_data(EMULE_UPLOAD_DEVICE* upload_device);

_int32 emule_upload_read_data_callback(struct tagTMP_FILE* file_struct, void *p_user, struct _tag_range_data_buffer* data_buffer, _int32 read_result, _u32 read_len);

BOOL emule_upload_fill_data(EMULE_UPLOAD_DEVICE* upload_device);

#endif
#endif /* ENABLE_EMULE */
#endif


