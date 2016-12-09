/*----------------------------------------------------------------------------------------------------------
author:		ZHANGSHAOHAN
created:	2008/08/24
-----------------------------------------------------------------------------------------------------------*/
#ifndef _SPEED_CAlCULATOR_H_
#define _SPEED_CAlCULATOR_H_
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"

#define		CALCULATOR_CYCLE			6

typedef struct tagSPEED_CALCULATOR
{
	_u32	_start_index;
	_u32	_end_index;
	_u32	_samples[CALCULATOR_CYCLE];
}SPEED_CALCULATOR;


// cycle 表示计算速度时，从当前时间往前推，参与计算的样本数
// unit 表示每个样本收集数据的时间跨度，单位毫秒
_int32 init_speed_calculator(SPEED_CALCULATOR* calculator, _u32 cycle, _u32 unit);

_int32 uninit_speed_calculator(SPEED_CALCULATOR* calculator);

void update_speed_calculator(SPEED_CALCULATOR* calculator, _u32 index);

_int32 add_speed_record(SPEED_CALCULATOR* calculator, _u32 bytes);

_int32 calculate_speed(SPEED_CALCULATOR* calculator, _u32* speed); // 获得当前的速度，单位是字节/秒
#ifdef __cplusplus
}
#endif
#endif

