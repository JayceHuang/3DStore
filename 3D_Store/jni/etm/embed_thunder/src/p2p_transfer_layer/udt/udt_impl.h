/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/11/24
-----------------------------------------------------------------------------------------------------------*/
#ifndef	_UDT_IMPL_H_
#define	_UDT_IMPL_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "p2p_transfer_layer/udt/udt_device.h"
#include "asyn_frame/msg.h"

struct tagSYN_CMD;

#define	SYN_TIMEOUT		(4000)
#define	SYN_MAX_RETRY		(10)


_int32 udt_init_global_bitmap(void);

_int32 udt_uninit_global_bitmap(void);

_int32 udt_notify_connect_result(UDT_DEVICE* udt, _int32 errcode);

//这是一个主动连接
_int32 udt_active_connect(UDT_DEVICE* udt, _u32 remote_ip, _u16 remote_port);

//这是一个被动连接
_int32 udt_passive_connect(UDT_DEVICE* udt, SYN_CMD* cmd, _u32 remote_ip, _u32 remote_port);

void udt_change_state(UDT_DEVICE* udt, UDT_STATE state);

_int32 udt_sendto(UDT_DEVICE* udt, UDT_SEND_BUFFER* send_buffer);

_int32 udt_sendto_callback(_int32 errcode, _u32 pending_op_count, const char* buffer, _u32 had_send, void* user_data);

void udt_update_waiting_send_queue(UDT_DEVICE* udt);

void udt_update_had_send_queue(UDT_DEVICE* udt);

void udt_update_recv_buffer_set(UDT_DEVICE* udt);

//pkt_seq为新版udt才有效的参数，旧版该值为-1
_int32 udt_handle_data_package(UDT_DEVICE* udt, char** buffer, char* data, _u32 data_len, _u32 seq, _u32 ack, _u32 wnd, _u32 pkt_seq);

_int32 udt_handle_ack_answer(UDT_DEVICE* udt, _u32 seq, _u32 ack, _u32 recv_wnd, _u32 last_recv_seq, _u32 pkt_base, char* bit, _u32 bit_count);	//处理对端回复的ack信息

_int32 udt_resend_package(UDT_DEVICE* udt, UDT_SEND_BUFFER* send_buffer);		//重新发送指定的数据包

_int32 udt_handle_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

_int32 udt_send_keepalive(UDT_DEVICE* udt);

_int32 udt_send_reset(UDT_DEVICE* udt);

_int32 udt_send_ack_answer(UDT_DEVICE* udt);

_int32 udt_recv_advance_ack_cmd(UDT_DEVICE* udt, ADVANCED_ACK_CMD* cmd);

_int32 udt_recv_keepalive(UDT_DEVICE* udt);

_int32 udt_recv_reset(UDT_DEVICE* udt);

_int32 udt_recv_syn_cmd(UDT_DEVICE* udt, struct tagSYN_CMD* cmd);

_int32 udt_handle_syn_timeout(const MSG_INFO *msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);

/*--------------------------------------------------------------------------------------------------------------------------------------
以下部分是辅助函数的实现
----------------------------------------------------------------------------------------------------------------------------------------*/
BOOL udt_is_seq_in_recv_window(UDT_DEVICE* udt, _u32 seq, _u32 data_len);

BOOL udt_is_seq_in_send_window(UDT_DEVICE* udt, _u32 seq);

BOOL udt_is_ack_in_send_window(UDT_DEVICE* udt, _u32 ack, _u32 recv_wnd);

_u32 udt_get_remain_send_window(UDT_DEVICE* udt);		//获取可以发送的数据大小

_int32 udt_fill_package_header(UDT_DEVICE* udt, char* cmd_buffer, _int32 cmd_buffer_len, _int32 data_len);

void udt_update_real_send_window(UDT_DEVICE* udt);	// 更新实际发送窗口

void udt_update_next_recv_seq(UDT_DEVICE* udt);		//更新next_recv_seq

void udt_get_lost_packet_bitmap(UDT_DEVICE* udt, BITMAP* bitmap);

void udt_update_last_recv_package_time(UDT_DEVICE* udt);	//更新最后一次收包的时间

void udt_update_last_send_package_time(UDT_DEVICE* udt);	//更新最后一次发包的时间

void udt_update_rtt(UDT_DEVICE* udt, _u32 seq, _u32 last_recv_seq, _u32 last_send_time);	//更新rtt数据

void udt_print_bitmap_pkt_info(UDT_DEVICE* udt, _u32 pkt_base, BITMAP* bitmap);

void udt_set_udp_buffer_low(BOOL value);

//这个用来通知上层之前的数据已发送成功，可以继续发送数据
_int32 udt_notify_ptl_send_callback(UDT_DEVICE* udt);
#ifdef __cplusplus
}
#endif
#endif
