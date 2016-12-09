// RTT round-trip time 发送报文与接收到另一端的响应之间的时延
// 说明：
// _round_trip_time_value是往返时延（raw_rtt）的加权平均值，权重为7^(n-1)/8^n，从最新的数起。
// _round_trip_deviation_estimator是往返时延变化的加权平均值，权重为3^(n-1)/4^n
// 最终的RTT为 _round_trip_time_value + _round_trip_deviation_estimator * 4
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
		// 超过一定次数后，每次翻倍。
		if(++rtt->_continuous_timeout_times > TIMEOUT_TIMES_THRESH)
		{
			rtt->_beta *= 2;
			rtt->_beta = MIN(rtt->_beta, MAX_BETA);
		}
	}
	else
	{
		rtt->_continuous_timeout_times = 0;
		rtt->_beta = MIN_BETA; // 直接赋最小值？？
	}
}

// 更新RTT
void rtt_update(NORMAL_RTT* rtt, _u32 raw_rtt)
{
	_int32 delta;
	if(raw_rtt == 0)
		raw_rtt = 1;
	if(!rtt->_rtt_first_updated) // 如果是第一次更新
	{
		rtt->_round_trip_time_value = MAX(raw_rtt, MIN_RTT);
		rtt->_round_trip_deviation_estimator = raw_rtt;
		rtt->_rtt_first_updated = TRUE;
		return;
	}
	delta = raw_rtt - rtt->_smoothed_round_trip_time;
	rtt->_smoothed_round_trip_time += (delta/8);		//旧7新1
	delta = sd_abs(delta);
	rtt->_round_trip_deviation_estimator += (delta - rtt->_round_trip_deviation_estimator) / 4; // 旧3新1
	// 更新RTO的值
	rtt->_round_trip_time_value = rtt->_smoothed_round_trip_time + (rtt->_round_trip_deviation_estimator * 4);
	if(rtt->_round_trip_time_value > RETRANSMIT_MAX_INTERVAL)
		rtt->_round_trip_time_value = RETRANSMIT_MAX_INTERVAL;
}

_u32 rtt_get(NORMAL_RTT* rtt)	//获取RTT
{
	return rtt->_round_trip_time_value;
}

_u32 rtt_get_retransmit_timeout(NORMAL_RTT* rtt)	// 获取超时重传时间阀值
{
	return rtt->_beta * rtt->_round_trip_time_value;
}
