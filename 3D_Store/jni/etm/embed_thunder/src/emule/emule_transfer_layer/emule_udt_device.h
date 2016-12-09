/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/11/30
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_UDT_DEVICE_H_
#define _EMULE_UDT_DEVICE_H_
#include "utility/define.h"
#ifdef ENABLE_EMULE
#ifdef ENABLE_EMULE_PROTOCOL

#include "emule/emule_utility/emule_define.h"
#include "emule/emule_transfer_layer/emule_udt_send_queue.h"
#include "emule/emule_transfer_layer/emule_udt_recv_queue.h"
#include "emule/emule_transfer_layer/emule_transfer_strategy.h"
#include "utility/list.h"
#include "utility/map.h"

struct tagEMULE_SOCKET_DEVICE;

typedef enum tagEMULE_UDT_STATE
{
	UDT_IDLE = 0,		// ����״̬
	UDT_TRAVERSE,		//��͸����״̬
//	UDT_SYN_SENT,    	// ���������ӣ���������SYN��Ϣ
//	UDT_SYN_RCVD,     	// �յ��������ӣ���������SYN��ACK��Ϣ
	UDT_ESTABLISHED,  	// ���ӽ���
	UDT_RESET
}EMULE_UDT_STATE;

typedef struct tagEMULE_UDT_SLIDE_WND
{
	_u32	_unack_wnd_size;		// �ѷ��͵���û�յ�ȷ�ϵ����ݳ���
	_u32	_congestion_wnd;		// ��������ӵ����������Ƴ�����ӵ������
	_u32	_advertised_wnd;		// ���շ����ݻ�������С������Ĵ��ڴ�С
}EMULE_UDT_SLIDE_WND;

typedef struct tagEMULE_UDT_DEVICE
{
	_u8							_conn_id[USER_ID_SIZE];		// ���ӱ�ʶ
	_u32						_conn_num;					//�����һ�����������������������ʶһ�����ӣ�����Ϊ0
	EMULE_TRANSFER_STRATEGY*	_transfer;
	EMULE_UDT_STATE			_state;
	_u32						_ip;
	_u16						_udp_port;
	EMULE_UDT_SEND_QUEUE*		_send_queue;
	EMULE_UDT_RECV_QUEUE*		_recv_queue;				//�հ�����
	struct tagEMULE_SOCKET_DEVICE*	_socket_device;
	_u32						_timeout_id;					
	_u32						_last_recv_time;				//���һ���յ����ݰ���ʱ��
	_u32						_last_send_keepalive_time;	//���һ�η���KEEPALIVE��ʱ��
}EMULE_UDT_DEVICE;

_int32 emule_udt_device_create(EMULE_UDT_DEVICE** udt_device);

_int32 emule_udt_device_close(EMULE_UDT_DEVICE* udt_device);

_int32 emule_udt_device_destroy(EMULE_UDT_DEVICE* udt_device);

_int32 emule_udt_device_connect(EMULE_UDT_DEVICE* udt_device, _u32 ip, _u16 port, _u8 conn_id[USER_ID_SIZE]);

_int32 emule_udt_device_notify_traverse(EMULE_UDT_DEVICE* udt_device, _int32 errcode);

_int32 emule_udt_device_send(EMULE_UDT_DEVICE* udt_device, char* buffer, _int32 len, BOOL is_data);

_int32 emule_udt_device_recv(EMULE_UDT_DEVICE* udt_device, char* buffer, _int32 len, BOOL is_data);

#endif
#endif /* ENABLE_EMULE */
#endif


