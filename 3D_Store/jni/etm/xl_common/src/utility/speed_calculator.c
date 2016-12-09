#include "utility/speed_calculator.h"
#include "utility/time.h"
#include "utility/errcode.h"
#include "utility/mempool.h"
#include "utility/string.h"
#include "utility/utility.h"
#include "utility/logid.h"
#define  LOGID	LOGID_COMMON
#include "utility/logger.h"

#define		CALCULATOR_UNIT				500
#define		SPEED_INDEX_CYCLE		8589920			//(0xFFFFFFFF / 500) =  8589934 ��Ϊ�˺�calculator->_cycle = 20�Ե��ϣ�������й���

#define		SPEED_INDEX_LT(a, b)	((a + SPEED_INDEX_CYCLE - b) % SPEED_INDEX_CYCLE > SPEED_INDEX_CYCLE / 2)
#define		SPEED_INDEX_GT(a, b)	((a + SPEED_INDEX_CYCLE - b) % SPEED_INDEX_CYCLE < SPEED_INDEX_CYCLE / 2)

_int32 init_speed_calculator(SPEED_CALCULATOR* calculator, _u32 cycle, _u32 unit)
{
	_u32 now;
	sd_memset(calculator, 0, sizeof(SPEED_CALCULATOR));
	sd_time_ms(&now);
	calculator->_start_index = (now / CALCULATOR_UNIT) % SPEED_INDEX_CYCLE;
    calculator->_end_index = (calculator->_start_index + 1) % SPEED_INDEX_CYCLE;
	return SUCCESS;
}

_int32 uninit_speed_calculator(SPEED_CALCULATOR* calculator)
{
	return SUCCESS;
}

void clean_speed_data(SPEED_CALCULATOR* calculator, _u32 start_index, _u32 end_index)
{
	_u32 unit_count, i;
	//if (start_index >= end_index)
	//{
	//	return;
	//}
	if(start_index == end_index || SPEED_INDEX_GT(start_index, end_index))
		return;
	unit_count = (end_index + SPEED_INDEX_CYCLE - start_index) % SPEED_INDEX_CYCLE;
	unit_count = MIN(unit_count, CALCULATOR_CYCLE);
	for(i = 0; i < unit_count; ++i)
	{
		calculator->_samples[(start_index + i) % CALCULATOR_CYCLE] = 0;
	}
}

void update_speed_calculator(SPEED_CALCULATOR* calculator, _u32 index)
{
	clean_speed_data(calculator, calculator->_end_index, (index + 1) % SPEED_INDEX_CYCLE);
	calculator->_end_index = (index + 1) % SPEED_INDEX_CYCLE;
	//�޷������ڽ��бȽϵ�ʱ����ʹ������Ľ�����бȽϣ�����������Խ�絼����ֵĴ��� 
//	if(calculator->_start_index + calculator->_cycle < calculator->_end_index)
	if(SPEED_INDEX_LT( ((calculator->_start_index + CALCULATOR_CYCLE) % SPEED_INDEX_CYCLE), calculator->_end_index))
	{
		calculator->_start_index = (calculator->_end_index + SPEED_INDEX_CYCLE - CALCULATOR_CYCLE) % SPEED_INDEX_CYCLE;
	}
}

_int32 add_speed_record(SPEED_CALCULATOR* calculator, _u32 bytes)
{
	_u32 now = 0, index = 0;
	sd_time_ms(&now);
	index = (now / CALCULATOR_UNIT) % SPEED_INDEX_CYCLE;
	update_speed_calculator(calculator, index);
	calculator->_samples[index % CALCULATOR_CYCLE] += bytes;
//	sd_assert(calculator->_end_index <= calculator->_start_index + calculator->_cycle);
	sd_assert(calculator->_end_index == (calculator->_start_index + CALCULATOR_CYCLE) % SPEED_INDEX_CYCLE || SPEED_INDEX_LT(calculator->_end_index, ((calculator->_start_index + CALCULATOR_CYCLE) % SPEED_INDEX_CYCLE)));
	return SUCCESS;
}

_int32 calculate_speed(SPEED_CALCULATOR* calculator, _u32* speed)
{
	_u32 unit_count, i, index, now;
	_u64 total = 0;
	sd_time_ms(&now);
	index = (now / CALCULATOR_UNIT) % SPEED_INDEX_CYCLE;
    update_speed_calculator(calculator, index);
	LOG_DEBUG("calculate_speed, start_index = %u, end_index = %u.", calculator->_start_index, calculator->_end_index);
//	if(calculator->_end_index < calculator->_start_index || calculator->_end_index == calculator->_start_index)
	if(SPEED_INDEX_LT(calculator->_end_index, calculator->_start_index) || calculator->_end_index == calculator->_start_index)
		return SUCCESS;		/*�������ã�ʱ����С�ڻ����0���ٶȲ���*/
	unit_count = (calculator->_end_index + SPEED_INDEX_CYCLE - calculator->_start_index) % SPEED_INDEX_CYCLE;
	sd_assert(unit_count <= CALCULATOR_CYCLE);
	if(unit_count == 0)
	{
		LOG_ERROR("[calculator = 0x%x]calculate_speed failed, unit_count == 0, start_index = %u, end_index = %u.", calculator, calculator->_start_index, calculator->_end_index);
		sd_assert(FALSE);
		return SUCCESS;
	}
	for(i = 0; i < unit_count; ++i)
	{
	    total += (calculator->_samples[(calculator->_start_index + i) % CALCULATOR_CYCLE]);
	}
	*speed = (total * 1000) / (unit_count * CALCULATOR_UNIT);
	LOG_DEBUG("calculate_speed, total = %llu, unit_count = %u, unit = %u, speed = %u.", total, unit_count, CALCULATOR_UNIT, *speed);
	return SUCCESS;
}

