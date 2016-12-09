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
	_u32 _remote_req_times;	//�Է������package�Ĵ�����Ϊ3ʱ�������ط���buffer������
	_u32 _last_send_time;	//�ϴη���ʱ��
	_u32 _package_seq;
	_u32 _reference_count;	//���ü���,ֻ�е�Ϊ0ʱ���ſ��Ի��ոÿ��ڴ�
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
	_u32 _remote_ip;						//�Զ˵�ip
	_u16 _remote_port;					//�Զ˵�udp�˿�
	_u32 _real_send_window;				//����������ʵ�ʵķ��ʹ��ڴ�С
	_u32 _send_window;					//���ʹ��ڴ�С,�Է�֪ͨ�Ľ��մ��ڴ�С
	_u32 _recv_window;					//���մ��ڴ�С
	_u32 _next_send_seq;					//�´η��Ͱ������	
	_u32 _unack_send_seq;				//�����δ���ȷ�ϵİ������
	_u32 _next_recv_seq;                 //�������յ���һ�����
	SLOW_START_CCA* _cca;						//ӵ������
	NORMAL_RTT* _rtt;							//��ʱ�ش�
	LIST _waiting_send_queue;			//�ȴ����͵Ķ��У�Ԫ��ΪUDT_SEND_BUFFER
	LIST _had_send_queue;				//�Ѿ����ͣ���δȷ�ϣ�Ԫ��ΪUDT_SEND_BUFFER(ע�⣬�����LIST����ײ��ʵ�����Ƚ��ȳ���)
	
	RECV_TYPE _recv_type;						//���պ������͵����ݣ��ڻص�ʱʹ��
	char* _recv_buffer;					//�������ݵĻ�����
	_u32 _expect_recv_len;				//�ڴ����յĳ���
	_u32 _had_recv_len;					//�Ѿ����յĳ���
	_u32 _start_read_seq;				//��ʼ��ȡ��λ��
	SET _udt_recv_buffer_set;			//�Ѿ����յ���udp���ݰ�,����ΪUDT_RECV_BUFFER
	_u32 _timeout_id;	
	BOOL _delay_ack_time_flag;			//��ʱӦ���Ƿ���Ч��־
	_u32 _delay_ack_time;				//�ӳ�Ӧ��ʱ�䣬������ʱ���ʾ���뷢��ackӦ���
	_u32 _last_recv_package_time;		//���һ�ν������ݰ���ʱ��
	_u32 _last_send_package_time;		//���һ�η������ݰ���ʱ��
	
	//�����ֶ������°汾udt
	_u32 _next_send_pkt_seq;				//��һ�����͵İ������
	_u32 _next_recv_pkt_seq;				//��һ�����հ������
	_u32 _max_recved_remote_pkt_seq;		//�ӶԷ��Ѿ��յ��ģ�����_udt_recv_buffer_set�е����һ��Ԫ�ص�pkt_seqֵ
	_u32 _last_recv_seq;					//���һ�ν��յ���seq���

	//��������
	_u32 _notify_send_msgid;
	_u32 _send_len_record;				//���������¼�ϲ���Ҫ���͵����ݳ��ȣ����ص�ʱʹ��
	_u32 _notify_recv_msgid;
}UDT_DEVICE;
#ifdef __cplusplus
}
#endif
#endif


