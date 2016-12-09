#ifndef _SD_NOTICE_QUEUE_H_00138F8F2E70_200807092122
#define _SD_NOTICE_QUEUE_H_00138F8F2E70_200807092122

#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/queue.h"
#include "asyn_frame/notice.h"

typedef struct {
	QUEUE _data_queue;
	_int32 _notice_handle;		//管道写端的文件描述符，pipe[1]
	_int32 _waitable_handle;		//管道读端的文件描述符，pipe[0]

} NOTICE_QUEUE;

_int32 notice_queue_init(NOTICE_QUEUE *queue, _int32 min_queue_count);

_int32 notice_queue_uninit(NOTICE_QUEUE *queue);


/*
 * Return immediately, even if there were not any msg popped.
 * If no msg popped, the *msg will be NULL.
 */
_int32 pop_notice_node(NOTICE_QUEUE *queue, void **data);


/*
 *	
 */
_int32 push_notice_node(NOTICE_QUEUE *queue, void *data);



_int32 pop_notice_node_without_dealloc(NOTICE_QUEUE *queue, void **data);


_int32 push_notice_node_without_alloc(NOTICE_QUEUE *queue, void *data);


#ifdef __cplusplus
}
#endif

#endif
