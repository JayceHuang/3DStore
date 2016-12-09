#ifdef LINUX
#ifdef MACOS

#ifndef NOTICE_MACOS_H_20130729
#define NOTICE_MACOS_H_20130729

#ifdef __cplusplus
extern "C" 
{
#endif

#include <semaphore.h>

typedef struct _SEVENT_HANDLE
{
	_u32 _index;
	sem_t*  sem;
	_int32 _value;
}SEVENT_HANDLE;

_int32 init_event_module(void);
_int32 uninit_event_module(void);


#ifdef __cplusplus
}
#endif

#endif

#endif
#endif

