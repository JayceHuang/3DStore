#ifndef _CALC_HASH_HELPER_20131220_H_H
#define _CALC_HASH_HELPER_20131220_H_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "utility/define.h"
#include "utility/list.h"

struct tag_CHH_HASH_INFO;

typedef _int32 (*notify_calc_hash_result)(struct tag_CHH_HASH_INFO*, void* user_data);

typedef struct tag_CHH_HASH_INFO
{
    _u32 _block_no;
    _u32 _data_len;
    _u8* _data_buffer;
    _u32 _hash_result_len;
    _u8* _hash_result;
}CHH_HASH_INFO;




/* every file_info only have one hash_calculator*/
typedef struct tag_HASH_CALCULATOR
{
    LIST _need_calc_hash_info_list;
    CHH_HASH_INFO* _cur_pending_item;
    void* _user_data;
    notify_calc_hash_result _callback;
    _u32 _msg_id;
}HASH_CALCULATOR;

_int32 chh_init(HASH_CALCULATOR* calculator
                , void* user_data
                , notify_calc_hash_result callback_func);
_int32 chh_uninit(HASH_CALCULATOR* calculator);
_int32 chh_insert_item(HASH_CALCULATOR* calculator, CHH_HASH_INFO* item);

_int32 chh_cancel_all_items(HASH_CALCULATOR* calculator);

_int32 chh_can_put_item(HASH_CALCULATOR* calculator);

#ifdef __cplusplus
}
#endif

#endif //_CALC_HASH_HELPER_20131220_H_H