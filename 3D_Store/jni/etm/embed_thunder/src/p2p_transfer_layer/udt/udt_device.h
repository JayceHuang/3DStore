#ifndef _UDT_DEVICE_H_
#define _UDT_DEVICE_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/list.h"
#include "utility/map.h"

#include "p2p_transfer_layer/ptl_interface.h"
#include "p2p_transfer_layer/udt/udt_slow_start_cca.h"
#include "p2p_transfer_layer/udt/rtt_calculator.h"
#include "p2p_transfer_layer/udt/udt_cmd_define.h"

typedef struct tagCONN_ID
{
	_u16 _virtual_source_port;
	_u16 _virtual_target_port;
	_u32 _peerid_hashcode;
}CONN_ID;

typedef enum tagUDT_STATE
{
	INIT_STATE = 0,
	SYN_STATE,
	SYN_ACK_STATE,
	ESTABLISHED_STATE,
	CLOSE_STATE,
}UDT_STATE;

typedef enum tagRECV_TYPE
{
	RECV_CMD_TYPE = 0,
	RECV_DATA_TYPE,
	RECV_DISCARD_DATA_TYPE,
}RECV_TYPE;

typedef	struct tagUDT_SEND_BUFFER 
{
	char* _buffer;
	_u32 _buffer_len;
	_u32 _seq;
	_u32 _data_len;
	_u32 _retry_time;
	_u32 _remote_req_times;	//对方请求该package的次数，为3时，马上重发该buffer的数据
	_u32 _last_send_time;	//上次发送时间
	_u32 _package_seq;
	_u32 _reference_count;	//引用计数,只有当为0时，才可以回收该块内存
	BOOL _is_data;
}UDT_SEND_BUFFER;

typedef struct tagUDT_RECV_BUFFER
{
	_u32 _seq;
	char* _buffer;
	char* _data;
	_u32 _data_len;
	_u32 _package_seq;
}UDT_RECV_BUFFER;

typedef struct tagUDT_DEVICE
{
	CONN_ID _id;
	UDT_STATE _state;
	PTL_DEVICE* _device;
	_u32 _syn_retry_times;
	_u32 _remote_ip;						//对端的ip
	_u16 _remote_port;					//对端的udp端口
	_u32 _real_send_window;				//经过调整后实际的发送窗口大小
	_u32 _send_window;					//发送窗口大小,对方通知的接收窗口大小
	_u32 _recv_window;					//接收窗口大小
	_u32 _next_send_seq;					//下次发送包的序号	
	_u32 _unack_send_seq;				//最早的未获得确认的包的序号
	_u32 _next_recv_seq;                 //期望接收的下一个序号
	SLOW_START_CCA* _cca;						//拥塞控制
	NORMAL_RTT* _rtt;							//超时重传
	LIST _waiting_send_queue;			//等待发送的队列，元素为UDT_SEND_BUFFER
	LIST _had_send_queue;				//已经发送，但未确认，元素为UDT_SEND_BUFFER(注意，这里的LIST假设底层的实现是先进先出的)
	
	RECV_TYPE _recv_type;						//接收何种类型的数据，在回调时使用
	char* _recv_buffer;					//接收数据的缓冲区
	_u32 _expect_recv_len;				//期待接收的长度
	_u32 _had_recv_len;					//已经接收的长度
	_u32 _start_read_seq;				//开始读取的位置
	SET _udt_recv_buffer_set;			//已经接收到的udp数据包,类型为UDT_RECV_BUFFER
	_u32 _timeout_id;	
	BOOL _delay_ack_time_flag;			//延时应答是否有效标志
	_u32 _delay_ack_time;				//延迟应答时间，超过该时间表示必须发送ack应答包
	_u32 _last_recv_package_time;		//最后一次接收数据包的时间
	_u32 _last_send_package_time;		//最后一次发送数据包的时间
	
	//以下字段用于新版本udt
	_u32 _next_send_pkt_seq;				//下一个发送的包的序号
	_u32 _next_recv_pkt_seq;				//下一个接收包的序号
	_u32 _max_recved_remote_pkt_seq;		//从对方已经收到的，存在_udt_recv_buffer_set中的最后一个元素的pkt_seq值
	_u32 _last_recv_seq;					//最后一次接收到的seq序号

	//辅助参数
	_u32 _notify_send_msgid;
	_u32 _send_len_record;				//这个用来记录上层需要发送的数据长度，供回调时使用
	_u32 _notify_recv_msgid;
}UDT_DEVICE;
#ifdef __cplusplus
}
#endif
#endif


