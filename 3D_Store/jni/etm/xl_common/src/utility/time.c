
#include "utility/time.h"
#include "utility/errcode.h"
#include "utility/string.h"
#include "utility/utility.h"

#define  LOGID LOGID_COMMON
#include "utility/logger.h"


#if defined(MACOS)
#include "platform/ios_interface/sd_ios_time_wrapper.h"
#endif


#if defined(LINUX)
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/times.h>

#ifndef MACOS
#define TIMES_TIME
#endif

/*#ifdef  TSC_TIME
#define rdtsc(low,high) __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))
#else
#define rdtsc(low,high)  
#endif
*/
#if defined(TSC_TIME)

/* unit: jiffies/ms */
static _u32 g_cpu_frq = 0;

/* 4G/g_cpu_frq */
static _u32 g_cpu_frq_divisor = 0;

static _u64 MAX_4G_VALUE=0xFFFFFFFFL;

 #define rdtsc(low,high) __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))
   
#elif defined(TIMES_TIME)

static _u32 TICK_PER_SEC = 100;



#endif

#elif defined(WINCE)
#include "platform/wm_interface/time_ce.h"
#include "utility/errcode.h"
#define time time_ce
#define time_t time_t_ce
#define localtime localtime_ce 
#endif

static BOOL g_is_get_base_time_ms = FALSE;
static _u32 BASE_TIME_SEC_MS = 0;


_int32 sd_localtime(_u32 time_sec, TIME_t *p_time)
{
#if defined(LINUX) || defined(WINCE)
#if defined(MACOS)
	sd_assert(FALSE);
#else
	struct tm *tm_time = NULL;
	time_t _time_sec = (time_t)time_sec;
	if (p_time)
	{
		sd_memset(p_time, 0, sizeof(*p_time));
		if ((_u32)-1 != _time_sec)
		{
			tm_time = localtime(&_time_sec);
			if (tm_time)
			{
				p_time->sec = tm_time->tm_sec;
				p_time->min = tm_time->tm_min;
				p_time->hour = tm_time->tm_hour;
				p_time->mday = tm_time->tm_mday;
				p_time->mon = tm_time->tm_mon;
				p_time->year = tm_time->tm_year + 1900;
				p_time->wday = tm_time->tm_wday;
				p_time->yday = tm_time->tm_yday;
//				p_time->isdst = tm_time->tm_isdst;
				return 0;
			}
		}
	}
#endif
	return 1;
#endif	
}

_int32 sd_local_time(TIME_t *p_time)
{
#if defined(LINUX) || defined(WINCE)
#if defined(MACOS)&&defined(MOBILE_PHONE)
	_int64 time_stamp = 0;
	_u32 msec = 0;
	_u32 wyear = 0;
	if (p_time)
	{
		sd_memset(p_time, 0, sizeof(*p_time));
		sd_ios_time_get_current_time_info(&time_stamp, &p_time->year,
											  &p_time->mon, &p_time->mday, &p_time->hour, &p_time->min, &p_time->sec, &msec,
											  &p_time->yday, &p_time->wday, &wyear);
	}	
#else
	time_t time_now = 0;
	struct tm *tm_now = NULL;
	if (p_time)
	{
		sd_memset(p_time, 0, sizeof(*p_time));
		time_now = time(NULL);
		if ((_u32)-1 != time_now)
		{
			tm_now = localtime(&time_now);
			if (tm_now)
			{
				p_time->sec = tm_now->tm_sec;
				p_time->min = tm_now->tm_min;
				p_time->hour = tm_now->tm_hour;
				p_time->mday = tm_now->tm_mday;
				p_time->mon = tm_now->tm_mon;
				p_time->year = tm_now->tm_year + 1900;
				p_time->wday = tm_now->tm_wday;
				p_time->yday = tm_now->tm_yday;
//				p_time->isdst = tm_now->tm_isdst;
			}
		}
		else
		{
			return 1;
		}
	}
#endif
#endif
	return 0;
}

_int32 sd_time(_u32 *times)
{
#if defined(LINUX)
	*times = time(NULL);
	if(*times == (_u32)-1)
	{
		return errno;
	}

#elif defined(WINCE)
	{
		SYSTEMTIME st;
		ULARGE_INTEGER	uli;
		FILETIME		ft;
		ULARGE_INTEGER	uli1970;
		FILETIME		ft1970;
		const SYSTEMTIME st1970 = {1970, 1,	4, 1, 0, 0, 0, 0};

		GetSystemTime(&st);

		// convert to a FILETIME
		// Gives number of 100-nanosecond intervals since January 1, 1601 (UTC)
		//
		if(!SystemTimeToFileTime(&st, &ft))
		{
			return -1;
		}

		// convert to a FILETIME
		// Gives number of 100-nanosecond intervals since January 1, 1970 (UTC)
		//
		if(!SystemTimeToFileTime(&st1970, &ft1970))
		{
			return -1;
		}

		// Copy file time structures into ularge integer so we can do
		// the math more easily
		//
		memcpy(&uli, &ft, sizeof(uli));
		memcpy(&uli1970, &ft1970, sizeof(uli1970));

		// Subtract the 1970 number of 100 ns value from the 1601 100 ns value
		// so we can get the number of 100 ns value between 1970 and now
		// then devide be 10,000,000 to get the number of seconds since 1970
		//
		uli.QuadPart = ((uli.QuadPart - uli1970.QuadPart) / 10000000);

		*times = (_u32)uli.QuadPart;
	}

#endif
	return SUCCESS;
}

_int32 sd_time_ms(_u32 *time_ms)
{

#if defined(LINUX)||defined(MACOS)

#if defined(TSC_TIME)
    _u32 hi,lo;
    //_u64 t = 1024*1024*1024;
    //t *= 4;
    rdtsc(lo, hi);

    //*time_ms = hi * g_cpu_frq_divisor + lo / g_cpu_frq;
    //*time_ms = ((_u64)hi * t + lo) / g_cpu_frq;
    *time_ms = ((_u64)hi * MAX_4G_VALUE + lo) / g_cpu_frq;
#elif defined(TIMES_TIME)

#ifdef COMMON_TIMES
    struct tms t_tms;
    _u32 ticks = times(&t_tms);
#else
    if( FALSE == g_is_get_base_time_ms)
    {
        _u32 now_time = times(NULL);
        if(now_time != -1)
            BASE_TIME_SEC_MS = now_time;
        g_is_get_base_time_ms = TRUE;
    }
    _u32 ticks = times(NULL) - BASE_TIME_SEC_MS;
#endif

    *time_ms = ticks*(1000/TICK_PER_SEC); 
    //LOG_DEBUG("sd_time_ms, ticks : %u, times : %u, TICK_PER_SEC : %u", ticks, *time_ms, TICK_PER_SEC);
#else

    if( FALSE == g_is_get_base_time_ms)
    {
        _int32 ret = 0;
        struct timeval now;
        ret = gettimeofday(&now, NULL);
        if(0 == ret)
            BASE_TIME_SEC_MS = now.tv_sec; 
        g_is_get_base_time_ms = TRUE;
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    *time_ms = (now.tv_sec - BASE_TIME_SEC_MS) * 1000 + now.tv_usec / 1000;
    //LOG_DEBUG("sd_time_ms gettimeofday : %u", *time_ms);
#endif

#elif defined(WINCE)
	//struct timeval now;
	_u32 nowtime = GetTickCount();
	*time_ms = nowtime;
#endif

	return SUCCESS;
}

void test_cpu_frq(void)
{
/* used for sd_time_ms in linux. need not implement in other os.
   at some customed board, we do not need to do this test.
     we can valuate g_cpu_frq&g_cpu_frq_divisor directly since we know its frequence */
#if defined(LINUX)

#if defined(TSC_TIME)

    _int32 i = 0;
    _u32 hi = 0;
   _u32 lo = 0;
   _u32 hi_1 = 0;
   _u32 lo_1 = 0;
    _u32 inv = 0, max_inv = 0, min_inv = 0xffffffff, sum_inv = 0;


    for(i = 0; i < 10;)
    {
        rdtsc(lo, hi);
        usleep(100);
        rdtsc(lo_1, hi_1);

        if(hi_1 - hi > 1)
        {
            /* invalid value,retry */
            continue;
        }

        /* assume jiffies per 100ms do not exceed to 4G, that means the cpu-frequence not exceed to 40G Hz */
        inv = lo_1 - lo;

        if(inv < min_inv)
            min_inv = inv;
        if(inv > max_inv)
            max_inv = inv;

        sum_inv += inv;

        i++;
    }

    /* remove the max & min, and calculate avg */
    sum_inv -= max_inv;
    sum_inv -= min_inv;

    g_cpu_frq_divisor = g_cpu_frq = sum_inv / 800;

    g_cpu_frq_divisor /= 4;
    if(g_cpu_frq_divisor > 0)
        g_cpu_frq_divisor = 1024 * 1024 * 1024 / g_cpu_frq_divisor;
    else
        g_cpu_frq_divisor = 0xffffffff;
   
#elif defined(TIMES_TIME)

 _int32 test_tick_per_sec = 0;
   test_tick_per_sec = sysconf(_SC_CLK_TCK);
   if(test_tick_per_sec != 0 && test_tick_per_sec != -1)
   {
        TICK_PER_SEC = test_tick_per_sec;
   }

#endif

#endif
}

char * time_int_to_str(_u32 it)
{
#ifdef MACOS
	//sd_assert(FALSE);
	return NULL;
#else
	static char st[64] = {0};
       struct tm *t_time=NULL;
	time_t tt=0;

	tt=it;
	sd_memset(st, 0, 64);
	
	t_time=localtime(&tt);

	sd_snprintf (st, 64, "%u-%u-%u %u:%u:%u",t_time->tm_year+1900,t_time->tm_mon+1,t_time->tm_mday,t_time->tm_hour,t_time->tm_min,t_time->tm_sec);

	return st;
#endif
}

_u32 time_str_to_int(char const* str)
{
#ifdef MACOS
	//sd_assert(FALSE);
	return 0;
#else
	_u32 it=0;
	char *st=(char *)str;
	struct tm t_time={0};
	char c_time[5]={0};
	
	sd_strncpy(c_time, st, 4);
	t_time.tm_year=sd_atoi(c_time)-1900;
	sd_memset(c_time,0,5);
	st+=5;

	sd_strncpy(c_time, st, 2);
	t_time.tm_mon=sd_atoi(c_time)-1;
	sd_memset(c_time,0,5);
	st+=3;

	sd_strncpy(c_time, st, 2);
	t_time.tm_mday=sd_atoi(c_time);
	sd_memset(c_time,0,5);
	st+=3;

	sd_strncpy(c_time, st, 2);
	t_time.tm_hour=sd_atoi(c_time);
	sd_memset(c_time,0,5);
	st+=3;

	sd_strncpy(c_time, st, 2);
	t_time.tm_min=sd_atoi(c_time);
	sd_memset(c_time,0,5);
	st+=3;

	sd_strncpy(c_time, st, 2);
	t_time.tm_sec=sd_atoi(c_time);
	sd_memset(c_time,0,5);
	
	it = (_u32)mktime(&t_time);
	
	return it;
#endif
}


