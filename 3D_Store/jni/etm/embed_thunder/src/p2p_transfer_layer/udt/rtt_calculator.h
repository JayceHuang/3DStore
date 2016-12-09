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
	_int32	_smoothed_round_trip_time;          // 平滑RTT
	_int32	_round_trip_deviation_estimator;
	_u32	_round_trip_time_value;				// 当前的RTT
	_u32	_continuous_timeout_times;			// 连续超时重传次数
	_u32	_beta;								// 计算超时重传时间阀值的系数
	BOOL	_rtt_first_updated;					// 是否第一次更新
}NORMAL_RTT;

void rtt_init(NORMAL_RTT* rtt);

void rtt_handle_retransmit(NORMAL_RTT* rtt, BOOL happened);	//通知重传与否

void rtt_update(NORMAL_RTT* rtt, _u32 raw_rtt);	//更新RTT

_u32 rtt_get(NORMAL_RTT* rtt);	//获取RTT

_u32 rtt_get_retransmit_timeout(NORMAL_RTT* rtt);	// 获取超时重传时间阀值
#ifdef __cplusplus
}
#endif
#endif
