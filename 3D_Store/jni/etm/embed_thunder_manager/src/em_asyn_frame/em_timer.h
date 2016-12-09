#ifndef _EM_TIMER_H_00138F8F2E70_200807031028
#define _EM_TIMER_H_00138F8F2E70_200807031028

#ifdef __cplusplus
extern "C" 
//{
#endif

#include "em_common/em_list.h"

#include "asyn_frame/timer.h"

_int32 em_init_timer(void);
_int32 em_uninit_timer(void);

_int32 em_refresh_timer(void);

_int32 em_pop_all_expire_timer(LIST *data);


/* timer_index:      TIMER_INDEX_INFINITE / TIMER_INDEX_NONE / (0 ---> (CYCLE_NODE-1)) */
_int32 em_put_into_timer(_u32 timeout, void *data, _int32 *time_index);

/* erase timernode by msgid, and return this msg 
 * Parameter: 
 *      timer_index:      TIMER_INDEX_INFINITE / TIMER_INDEX_NONE / (0 ---> (CYCLE_NODE-1))
 */
_int32 em_erase_from_timer(void *comparator_data, data_comparator comp_fun, _int32 timer_index, void **data);


/* @Simple Function@
 * Return :  current timestamp in timer
 */
_u32 em_get_current_timestamp(void);


#ifdef __cplusplus
//}
#endif

#endif
