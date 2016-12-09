#ifndef _WSA_EVENT_SELECT_H_00138F8F2E70_200100724
#define _WSA_EVENT_SELECT_H_00138F8F2E70_200100724
#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef _WSA_EVENT_SELECT

#ifndef WINCE
#error Compile error, must define WINCE since _WSA_EVENT_SELECT defined
#endif

#include "asyn_frame/selector.h"
#include <WINSOCK2.H>   

struct pollfd{
    _int32 fd;			/* File descriptor to poll.  */
   _int32 err_code;
    _int16 events;		/* Types of events poller cares about.  */
    _int16 revents;		/* Types of events that actually occurred.  */
};

typedef struct {
	_int32 _selector_size;
	_int32 _first_free_idx;
	_int32 _max_available_idx;
	_int32 _min_event_idx;
	struct pollfd *_channel_events;
	WSAEVENT *_wsa_events;
	CHANNEL_DATA *_channel_data;
} WES_SELECTOR;


#endif

#ifdef __cplusplus
}
#endif
#endif
