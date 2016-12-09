#ifndef SD_TIME_H_00138F8F2E70_200806161735
#define SD_TIME_H_00138F8F2E70_200806161735

#ifdef __cplusplus
extern "C" 
{
#endif

#include <time.h>
#include "utility/define.h"

typedef struct tagTIME_t
{
	_u32 sec;
	_u32 min;
	_u32 hour;
	_u32 mday;
	_u32 mon;
	_u32 year;
	_u32 wday;
	_u32 yday;
//	_u32 isdst;
} TIME_t, TIME;

#if (defined(_ANDROID_LINUX) && !defined(__GNUC__) || defined(OSX))
struct tm {
	int     tm_sec;         /* seconds after the minute [0-60] */
	int     tm_min;         /* minutes after the hour [0-59] */
	int     tm_hour;        /* hours since midnight [0-23] */
	int     tm_mday;        /* day of the month [1-31] */
	int     tm_mon;         /* months since January [0-11] */
	int     tm_year;        /* years since 1900 */
	int     tm_wday;        /* days since Sunday [0-6] */
	int     tm_yday;        /* days since January 1 [0-365] */
	int     tm_isdst;       /* Daylight Savings Time flag */
	long    tm_gmtoff;      /* offset from CUT in seconds */
	char    *tm_zone;       /* timezone abbreviation */
};
#endif /*  _ANDROID_LINUX */
_int32 sd_localtime(_u32 time_sec, TIME_t *p_time);

_int32 sd_local_time(TIME_t *p_time);


/* seconds from Epoch */
_int32 sd_time(_u32 *times);


/* timestamp
 * millisecond from a fixed point in time 
 * only used for computing INTERVAL
 */
_int32 sd_time_ms(_u32 *time_ms);

/* used for sd_time_ms in linux. need not implement in other os.
   at some customed board, we do not need to do this test.
     we can valuate g_cpu_frq&g_cpu_frq_divisor directly since we know its frequence */
void test_cpu_frq(void);


#define TIME_LT(st1, st2) ((_int32)((st1) - (st2)) < 0)
#define TIME_GT(st1, st2) ((_int32)((st1) - (st2)) > 0)
#define TIME_LE(st1, st2) ((_int32)((st1) - (st2)) <= 0)
#define TIME_GE(st1, st2) ((_int32)((st1) - (st2)) >= 0)

/**/
#define TIME_SUBZ(st1, st2) (TIME_LE((st1), (st2)) ? 0 : (st1) - (st2))

_u32 time_str_to_int(char const* st);
char * time_int_to_str(_u32 it);

#ifdef __cplusplus
}
#endif

#endif
