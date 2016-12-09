// RTT round-trip time ���ͱ�������յ���һ�˵���Ӧ֮���ʱ��
// ˵����
// _round_trip_time_value������ʱ�ӣ�raw_rtt���ļ�Ȩƽ��ֵ��Ȩ��Ϊ7^(n-1)/8^n�������µ�����
// _round_trip_deviation_estimator������ʱ�ӱ仯�ļ�Ȩƽ��ֵ��Ȩ��Ϊ3^(n-1)/4^n
// ���յ�RTTΪ _round_trip_time_value + _round_trip_deviation_estimator * 4
//////////////////////////////////////////////////////////////////////
#include "rtt_calculator.h"
#include "utility/utility.h"

#define		MIN_RTT					30
#define		MIN_BETA				2
#define		MAX_BETA				1024
#define		TIMEOUT_TIMES_THRESH	10
#define		INIT_RTT_ESTIMATOR		3000	//3 seconds
#define		RETRANSMIT_MAX_INTERVAL	15000	//15 seconds

void rtt_init(NORMAL_RTT* rtt)
{
	rtt->_continuous_timeout_times = 0;
	rtt->_beta = MIN_BETA;
	rtt->_smoothed_round_trip_time = 0;
	rtt->_round_trip_deviation_estimator = INIT_RTT_ESTIMATOR;
	rtt->_round_trip_time_value = rtt->_smoothed_round_trip_time + rtt->_round_trip_deviation_estimator;
	rtt->_rtt_first_updated = FALSE;
}

void rtt_handle_retransmit(NORMAL_RTT* rtt, BOOL happened)
{
	if(happened)
	{
		// ����һ��������ÿ�η�����
		if(++rtt->_continuous_timeout_times > TIMEOUT_TIMES_THRESH)
		{
			rtt->_beta *= 2;
			rtt->_beta = MIN(rtt->_beta, MAX_BETA);
		}
	}
	else
	{
		rtt->_continuous_timeout_times = 0;
		rtt->_beta = MIN_BETA; // ֱ�Ӹ���Сֵ����
	}
}

// ����RTT
void rtt_update(NORMAL_RTT* rtt, _u32 raw_rtt)
{
	_int32 delta;
	if(raw_rtt == 0)
		raw_rtt = 1;
	if(!rtt->_rtt_first_updated) // ����ǵ�һ�θ���
	{
		rtt->_round_trip_time_value = MAX(raw_rtt, MIN_RTT);
		rtt->_round_trip_deviation_estimator = raw_rtt;
		rtt->_rtt_first_updated = TRUE;
		return;
	}
	delta = raw_rtt - rtt->_smoothed_round_trip_time;
	rtt->_smoothed_round_trip_time += (delta/8);		//��7��1
	delta = sd_abs(delta);
	rtt->_round_trip_deviation_estimator += (delta - rtt->_round_trip_deviation_estimator) / 4; // ��3��1
	// ����RTO��ֵ
	rtt->_round_trip_time_value = rtt->_smoothed_round_trip_time + (rtt->_round_trip_deviation_estimator * 4);
	if(rtt->_round_trip_time_value > RETRANSMIT_MAX_INTERVAL)
		rtt->_round_trip_time_value = RETRANSMIT_MAX_INTERVAL;
}

_u32 rtt_get(NORMAL_RTT* rtt)	//��ȡRTT
{
	return rtt->_round_trip_time_value;
}

_u32 rtt_get_retransmit_timeout(NORMAL_RTT* rtt)	// ��ȡ��ʱ�ش�ʱ�䷧ֵ
{
	return rtt->_beta * rtt->_round_trip_time_value;
}
