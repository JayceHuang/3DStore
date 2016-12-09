#ifndef CALC_HASH_REACTOR_20131224_H_H
#define CALC_HASH_REACTOR_20131224_H_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "utility/define.h"
#include "msg.h"



_int32 init_calc_hash_reactor(_int32* waitable_handle);
_int32 uninit_calc_hash_reactor();

_int32 get_complete_calc_hash_msg(MSG **msg);
_int32 regist_calc_hash_event(MSG* msg);
_int32 unregister_calc_hash(MSG *msg, _int32 reason);

#ifdef __cplusplus
};
#endif

#endif //CALC_HASH_REACTOR_20131224_H_H