#ifndef _SD_POLL_WRAPPER_H_00138F8F2E70_200809021133
#define _SD_POLL_WRAPPER_H_00138F8F2E70_200809021133
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef _POLL

#ifndef LINUX
#error Compile error, must define LINUX since _POLL defined
#endif

#include <sys/poll.h>

typedef struct {
	_int32 _selector_size;
	_int32 _first_free_idx;
	_int32 _max_available_idx;
	struct pollfd *_channel_events;
	CHANNEL_DATA *_channel_data;
} POLL_SELECTOR;


#endif

#ifdef __cplusplus
}
#endif
#endif
