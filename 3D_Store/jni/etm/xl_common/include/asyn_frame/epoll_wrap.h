#ifndef _SD_EPOLL_WRAPPER_H_00138F8F2E70_200809021134
#define _SD_EPOLL_WRAPPER_H_00138F8F2E70_200809021134
#ifdef __cplusplus
extern "C" 
{
#endif

#include "utility/define.h"
#ifdef _EPOLL

#ifndef LINUX
#error Compile error, must define LINUX since _EPOLL defined
#endif

#include <sys/epoll.h>


typedef struct {
	_int32 _epoll_fd;
	_int32 _selector_size;
	struct epoll_event *_channel_events;
} EPOLL_SELECTOR;


#endif

#ifdef __cplusplus
}
#endif

#endif
