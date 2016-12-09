#ifndef _SD_KQUEUE_WRAPPER_H_00138F8F2E70_200809021133
#define _SD_KQUEUE_WRAPPER_H_00138F8F2E70_200809021133

#ifdef _KQUEUE

#include "utility/define.h"

#ifndef LINUX
#error Compile error, must define LINUX since _KQUEUE defined
#endif

#include <sys/event.h>
#include <sys/time.h> 

#if defined(LINUX)
struct pollfd{
    _int32 fd;			/* File descriptor to poll.  */
    _int16 events;		/* Types of events poller cares about.  */
    _int16 revents;		/* Types of events that actually occurred.  */
};
#endif

typedef struct {
    _int32 _kqueue_fd;
    _int32 _selector_size;
    _int32 _first_free_idx;
    _int32 _max_available_idx;
    struct pollfd    *_channel_events;
    CHANNEL_DATA *_channel_data;
} KQUEUE_SELECTOR;

#endif

#endif
