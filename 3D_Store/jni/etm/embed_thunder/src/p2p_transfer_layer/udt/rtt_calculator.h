/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/11/23
-----------------------------------------------------------------------------------------------------------*/
#ifndef	_RTT_CALCULATOR_H_
#define	_RTT_CALCULATOR_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

typedef struct tagNORMAL_RTT
{
	_int32	_smoothed_round_trip_time;          // ƽ��RTT
	_int32	_round_trip_deviation_estimator;
	_u32	_round_trip_time_value;				// ��ǰ��RTT
	_u32	_continuous_timeout_times;			// ������ʱ�ش�����
	_u32	_beta;								// ���㳬ʱ�ش�ʱ�䷧ֵ��ϵ��
	BOOL	_rtt_first_updated;					// �Ƿ��һ�θ���
}NORMAL_RTT;

void rtt_init(NORMAL_RTT* rtt);

void rtt_handle_retransmit(NORMAL_RTT* rtt, BOOL happened);	//֪ͨ�ش����

void rtt_update(NORMAL_RTT* rtt, _u32 raw_rtt);	//����RTT

_u32 rtt_get(NORMAL_RTT* rtt);	//��ȡRTT

_u32 rtt_get_retransmit_timeout(NORMAL_RTT* rtt);	// ��ȡ��ʱ�ش�ʱ�䷧ֵ
#ifdef __cplusplus
}
#endif
#endif
