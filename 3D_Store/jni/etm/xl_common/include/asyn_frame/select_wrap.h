#ifndef _SD_SELECT_WRAPPER_H_00138F8F2E70_200811201136
#define _SD_SELECT_WRAPPER_H_00138F8F2E70_200811201136
#ifdef __cplusplus
extern "C" 
{
#endif
#include "utility/define.h"
#ifdef _SELECT


#if defined(WINCE)
struct pollfd{
    _int32 fd;			/* File descriptor to poll.  */
   _int32 err_code;
    _int16 events;		/* Types of events poller cares about.  */
    _int16 revents;		/* Types of events that actually occurred.  */
};
#elif defined(LINUX)
struct pollfd{
    _int32 fd;			/* File descriptor to poll.  */
    _int16 events;		/* Types of events poller cares about.  */
    _int16 revents;		/* Types of events that actually occurred.  */
};

#endif

typedef struct {
	_int32 _selector_size;
	_int32 _first_free_idx;
	_int32 _max_aviable_idx;
	struct pollfd *_channel_events;
	CHANNEL_DATA *_channel_data;
} SELECT_SELECTOR;



#endif
#ifdef __cplusplus
}
#endif
#endif
