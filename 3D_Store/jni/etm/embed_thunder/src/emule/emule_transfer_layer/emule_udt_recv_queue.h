/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/12/8
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_UDT_RECV_QUEUE_H_
#define _EMULE_UDT_RECV_QUEUE_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL
#include "asyn_frame/msg.h"
#include "utility/map.h"

struct tagEMULE_UDT_DEVICE;

typedef struct tagEMULE_UDT_RECV_BUFFER
{
	_u32	_seq;
	char*	_buffer;
	char*	_data;
	_u32	_data_len;
}EMULE_UDT_RECV_BUFFER;

typedef struct tagEMULE_UDT_RECV_QUEUE
{
	char*						_buffer;
	_u32						_len;
	_u32						_offset;
	BOOL						_is_recv_data;
	_u32						_recv_msg_id;
	struct tagEMULE_UDT_DEVICE*	_udt_device;
	SET							_recv_buffer_set;
	_u32						_last_read_segment;
	_u32						_segment_offset;
	_u32						_recv_data_size;			//目前在recv_queue中有效数据的长度
}EMULE_UDT_RECV_QUEUE;

_int32 emule_udt_recv_queue_create(EMULE_UDT_RECV_QUEUE** recv_queue, struct tagEMULE_UDT_DEVICE* udt_device);

_int32 emule_udt_recv_queue_close(EMULE_UDT_RECV_QUEUE* recv_queue);

_int32 emule_udt_recv_queue_read(EMULE_UDT_RECV_QUEUE* recv_queue);

_int32 emule_recv_queue_notify_msg(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

BOOL emule_udt_recv_queue_lookup(EMULE_UDT_RECV_QUEUE* recv_queue, _u32 seq);

BOOL emule_udt_recv_queue_add(EMULE_UDT_RECV_QUEUE* recv_queue, _u32 seq, char** buffer, _u32 len);

_u32 emule_udt_recv_queue_size(EMULE_UDT_RECV_QUEUE* recv_queue);

#endif
#endif /* ENABLE_EMULE */
#endif




