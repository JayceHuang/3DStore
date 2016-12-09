/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/12/9
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_UDT_SEND_QUEUE_H_
#define _EMULE_UDT_SEND_QUEUE_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "asyn_frame/msg.h"
#include "utility/list.h"

struct tagEMULE_UDT_DEVICE;

#define		MAX_SEGMENT_SIZE		1300

typedef struct tagEMULE_UDT_SEND_REQ
{
	char*	_buffer;
	_u32	_len;
	LIST		_udt_send_buffer_list;
    BOOL    _is_data;
}EMULE_UDT_SEND_REQ;

typedef struct tagEMULE_UDT_SEND_BUFFER
{
	_u32	_seq_num;
	_u32	_send_time;
	_u32	_buffer_len;
	_u32	_data_len;
	_u32	_send_count;
	char		_buffer[MAX_SEGMENT_SIZE];
	BOOL    _is_data;
}EMULE_UDT_SEND_BUFFER;

typedef struct tagEMULE_UDT_SEND_QUEUE
{
	LIST							_waiting_send_queue;		//类型是emule_udt_send_req
	LIST							_had_send_queue;
	EMULE_UDT_SEND_REQ*		_cur_req;			//目前正在处理的请求
	struct tagEMULE_UDT_DEVICE*	_udt_device;
	_u32						_seq_num;			//序号
	_u32						_send_wnd;			//发送窗口(对端给的接收窗口)
	_u32						_congestion_wnd;	//拥塞窗口
	_u32						_slow_start_threshold;
	_u32						_pending_wnd;		//已经发送但还没被确认的数据大小
	_u32						_send_msg_id;
	//快速重传辅助参数
	_u32						_unack_count;
	//计算重传时间的参数
	_u32						_time_to_check;		//下一次检查的时间
	_u32						_backoff;
	_u32						_srtt;				// Smoothed round trip time, milliseconds 
	_u32						_mdev;				// Mean deviation, milliseconds  衡量rtt的变化速度。
}EMULE_UDT_SEND_QUEUE;

_int32 emule_udt_send_queue_create(EMULE_UDT_SEND_QUEUE** send_queue, struct tagEMULE_UDT_DEVICE* udt_device);

_int32 emule_udt_send_queue_close(EMULE_UDT_SEND_QUEUE* send_queue);

_int32 emule_udt_send_queue_add(EMULE_UDT_SEND_QUEUE* send_queue, char* data, _u32 data_len, BOOL is_data);

_int32 emule_udt_send_queue_remove(EMULE_UDT_SEND_QUEUE* send_queue, _u32 ack, _u32 wnd);

_int32 emule_udt_send_queue_timeout(EMULE_UDT_SEND_QUEUE* send_queue);

_int32 emule_udt_send_queue_update(EMULE_UDT_SEND_QUEUE* send_queue);

_int32 emule_udt_send_queue_msg(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

//这里可以进一步优化，即把所有的数据尽量填满一个segment后一起发送，目前没有，以后可以优化
_int32 emule_split_package_to_segment(EMULE_UDT_SEND_REQ* req, _u32* seq);
#endif
#endif /* ENABLE_EMULE */

#endif

