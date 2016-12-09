#ifdef WINCE

#ifndef SD_TASK_WINCE_H_20130730
#define SD_TASK_WINCE_H_20130730

#ifdef __cplusplus
extern "C" 
{
#endif

#include <windows.h>

typedef struct _LIST_ENTRY_SD {
	struct _LIST_ENTRY *Flink;
	struct _LIST_ENTRY *Blink;
} LIST_ENTRY_SD;

typedef struct _RTL_CRITICAL_SECTION_DEBUG_SD {
    _u16   Type;
    _u16   CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION_SD *CriticalSection;
    LIST_ENTRY_SD ProcessLocksList;
    _u32 EntryCount;
    _u32 ContentionCount;
    _u32 Spare[ 2 ];
} RTL_CRITICAL_SECTION_DEBUG_SD, *PRTL_CRITICAL_SECTION_DEBUG_SD, RTL_RESOURCE_DEBUG_SD, *PRTL_RESOURCE_DEBUG_SD;

typedef struct _RTL_CRITICAL_SECTION_SD {
    PRTL_CRITICAL_SECTION_DEBUG_SD DebugInfo;
	
    //  The following three fields control entering and exiting the critical section for the resource
    _int32 LockCount;
    _int32 RecursionCount;
    _int32 OwningThread;        // from the thread's ClientId->UniqueThread
    _int32 LockSemaphore;
    _u32* SpinCount;        // force size on 64-bit systems when packed
} RTL_CRITICAL_SECTION_SD, *PRTL_CRITICAL_SECTION_SD;

typedef RTL_CRITICAL_SECTION_SD CRITICAL_SECTION_SD;

typedef CRITICAL_SECTION_SD TASK_LOCK;
typedef CRITICAL_SECTION_SD TASK_COND;

#ifdef __cplusplus
}
#endif

#endif

#endif

