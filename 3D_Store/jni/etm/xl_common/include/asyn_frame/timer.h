#ifndef _SD_TIMER_H_00138F8F2E70_200807031028
#define _SD_TIMER_H_00138F8F2E70_200807031028

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/list.h"

/* UNIT: ms */

#define GRANULARITY		(10)
#define CYCLE_PERIOD	(1000)

#define	CYCLE_NODE		(CYCLE_PERIOD / GRANULARITY)

#define TIMER_INDEX_INFINITE	(-1)
#define TIMER_INDEX_NONE		(-2)


typedef struct t_time_node{
	_u32 _interval; /* interval from now */
	LIST _data;
	struct t_time_node *_next_node;

#ifdef _DEBUG
	_u32 _data_timestamp;
#endif

} TIME_NODE;

_int32 init_timer(void);
_int32 uninit_timer(void);

_int32 refresh_timer(void);

_int32 pop_all_expire_timer(LIST *data);


/* timer_index:      TIMER_INDEX_INFINITE / TIMER_INDEX_NONE / (0 ---> (CYCLE_NODE-1)) */
_int32 put_into_timer(_u32 timeout, void *data, _int32 *time_index);

/* erase timernode by msgid, and return this msg 
 * Parameter: 
 *      timer_index:      TIMER_INDEX_INFINITE / TIMER_INDEX_NONE / (0 ---> (CYCLE_NODE-1))
 */
_int32 erase_from_timer(void *comparator_data, data_comparator comp_fun, _int32 timer_index, void **data);


/* @Simple Function@
 * Return :  current timestamp in timer
 */
_u32 get_current_timestamp(void);


#ifdef __cplusplus
}
#endif

#endif
