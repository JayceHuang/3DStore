#ifndef _EM_QUEUE_H_00138F8F2E70_200806260949
#define _EM_QUEUE_H_00138F8F2E70_200806260949

#ifdef __cplusplus
extern "C" 
//{
#endif

#include "utility/queue.h"


#define em_queue_init queue_init
#define em_queue_uninit queue_uninit
#define em_queue_size queue_size
#define em_queue_push queue_push
#define em_queue_pop queue_pop
#define em_queue_push_without_alloc queue_push_without_alloc
#define em_queue_pop_without_dealloc queue_pop_without_dealloc
#define em_queue_reserved queue_reserved
#define em_queue_check_full queue_check_full

#ifdef __cplusplus
//}
#endif

#endif
