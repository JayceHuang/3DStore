
/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2009/09/28
-----------------------------------------------------------------------------------------------------------*/
#ifndef _EMULE_PIECE_CHECKER_H_
#define _EMULE_PIECE_CHECKER_H_

#include "utility/define.h"
#ifdef ENABLE_EMULE
#include "data_manager/data_receive.h"
#include "asyn_frame/msg.h"
#include "utility/md4.h"
#include "utility/list.h"

struct tagEMULE_DATA_MANAGER;
struct tagTMP_FILE;

#define		INVALID_PART_INDEX		-1


typedef struct tagEMULE_PART_CHECKER
{
	struct tagEMULE_DATA_MANAGER*	_data_manager;
	LIST								_part_index_list;			//需要校验的part_index
	_u32							_cur_hash_index;			//目前正在校验的part_index	
	RANGE_DATA_BUFFER				_data_buffer;
	RANGE							_part_range;	
	CTX_MD4						_hash_result;
	_u32							_part_hash_len;
	_u8*							_part_hash;
    
	_u32							_aich_hash_len;
	_u8*							_aich_hash;
    
	_u32							_delay_read_msgid;

	RANGE_DATA_BUFFER				_cross_data_buffer[2];
}EMULE_PART_CHECKER;

_int32 emule_create_part_checker(EMULE_PART_CHECKER* checker, struct tagEMULE_DATA_MANAGER* data_manager);

_int32 emule_close_part_checker(EMULE_PART_CHECKER* checker);

_int32 emule_checker_recv_range(EMULE_PART_CHECKER* checker, const RANGE* range);

_int32 emule_checker_init_check_range(EMULE_PART_CHECKER* checker);

_int32 emule_range_to_exclude_part_index(struct tagEMULE_DATA_MANAGER* data_manager, const RANGE* range, _u32* start_index, _u32* end_index);

_int32 emule_range_to_include_part_index(struct tagEMULE_DATA_MANAGER* data_manager, const RANGE* range, _u32* index, _u32* num);

//把part_index转为包含该完整part范围的range
_int32 emule_part_index_to_range(struct tagEMULE_DATA_MANAGER* data_manager, _u32 index, RANGE* part_range);

//把part转化为该part包含的range
_int32 emule_part_to_include_range(struct tagEMULE_DATA_MANAGER* data_manager, _u32 part_index, _u32 part_num, RANGE* part_range);

_int32 emule_check_next_part(EMULE_PART_CHECKER* checker);

_int32 emule_check_part_hash(EMULE_PART_CHECKER* checker);

_int32 emule_notify_part_hash_read (struct tagTMP_FILE* file, void* user_data, RANGE_DATA_BUFFER* data_buffer, _int32 result, _u32 read_len);

_int32 emule_calc_part_hash(EMULE_PART_CHECKER* checker, RANGE_DATA_BUFFER* data_buffer);

_int32 emule_verify_part_hash(EMULE_PART_CHECKER* checker);

_int32 emule_notify_delay_read(const MSG_INFO* msg_info, _int32 errcode, _u32 notice_count_left, _u32 elapsed, _u32 msgid);



void   emule_cache_cross_buffer(EMULE_PART_CHECKER* checker);

void  emule_save_cross_data(EMULE_PART_CHECKER* checker);

_u32  emule_once_check_range_num(EMULE_PART_CHECKER* checker);
#endif /* ENABLE_EMULE */
#endif


