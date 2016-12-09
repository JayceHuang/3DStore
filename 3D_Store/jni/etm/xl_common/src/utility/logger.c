#include "utility/define.h"
#include "utility/errcode.h"
#include "utility/arg.h"
#include "utility/logid.h"
#include "utility/string.h"
#include "platform/sd_customed_interface.h"
#include "platform/sd_fs.h"


#if defined(LINUX)
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#if defined(LINUX) && (!defined(_ANDROID_LINUX)) 
//#include <execinfo.h>
#endif

#include <time.h>
#include <sys/time.h>
//#if defined(MACOS)&&defined(MOBILE_PHONE)
//struct tm {
//	int     tm_sec;         /* seconds after the minute [0-60] */
//	int     tm_min;         /* minutes after the hour [0-59] */
//	int     tm_hour;        /* hours since midnight [0-23] */
//	int     tm_mday;        /* day of the month [1-31] */
//	int     tm_mon;         /* months since January [0-11] */
//	int     tm_year;        /* years since 1900 */
//	int     tm_wday;        /* days since Sunday [0-6] */
//	int     tm_yday;        /* days since January 1 [0-365] */
//	int     tm_isdst;       /* Daylight Savings Time flag */
//	long    tm_gmtoff;      /* offset from CUT in seconds */
//	char    *tm_zone;       /* timezone abbreviation */
//};
//#endif

#elif defined(WINCE)
#include <time.h>
#include <windows.h>
#include "utility/time.h"
//#include <sys/timeb.h>
#endif
#if defined(_DUOKAN)
#include <android/log.h>
#define ANDROID_LOG __android_log_print
#endif

_int32 logger_printf(const char *fmt, ...)
{
	struct tm localtime;
	struct timeval now;
	sd_va_list ap;

	sd_va_start(ap, fmt);
	gettimeofday(&now, NULL);
	localtime_r(&now.tv_sec, &localtime);

	printf("\n[%04d-%02d-%02d %02d:%02d:%02d:%03ld]: ", 
		localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
		localtime.tm_hour, localtime.tm_min, localtime.tm_sec, now.tv_usec / 1000);

	vprintf(fmt, ap);

	sd_va_end(ap);
	
	return 0;
}

#ifdef _RELEASE_LOG

#define RUNNING_LOG_FILE EMBED_THUNDER_TEMPORARY_DIR"running.log"
#define BACKUP_RUNNING_LOG_FILE EMBED_THUNDER_TEMPORARY_DIR"backup_running.log"

static volatile BOOL is_g_running_log_inited = FALSE;
static FILE *g_running_log_fp;

void close_running_log()
{
	if (is_g_running_log_inited) {
		fclose(g_running_log_fp);
		is_g_running_log_inited = FALSE;
	}
}

_int32 running_logger(const char *fmt, ...)
{
	struct tm localtime;
	struct timeval now;
	FILE *fp = g_running_log_fp;
	char buff[1024+1];
	sd_va_list ap;
	int n;

	if(FALSE == is_g_running_log_inited)
	{
        //如果running_log文件已存在，那么备份到同一目录，文件名为backup_running.log
		if(sd_file_exist(RUNNING_LOG_FILE))
		{
            if (sd_file_exist(BACKUP_RUNNING_LOG_FILE))
            {
                sd_delete_file(BACKUP_RUNNING_LOG_FILE);
            }
            sd_rename_file(RUNNING_LOG_FILE, BACKUP_RUNNING_LOG_FILE);        
		}
   		g_running_log_fp = fopen(RUNNING_LOG_FILE, "w+");
        
		if(NULL == g_running_log_fp) return SD_ERROR;
		if(SUCCESS != fseek(g_running_log_fp, 0, SEEK_END)) return SD_ERROR;
		fp = g_running_log_fp;
		is_g_running_log_inited = TRUE;
	}

	if(ftell(fp) >= 2*1024*1024)
	{ // 超过2M, 不再继续写running_log
		//rewind(fp);
        return 0;
	}

	sd_va_start(ap, fmt);
	gettimeofday(&now, NULL);
	localtime_r(&now.tv_sec, &localtime);

	n = snprintf(buff, sizeof(buff)-1, "\n[%d-%d-%d %d:%d:%d:%ld][pid:%u]: ", 
		localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
		localtime.tm_hour, localtime.tm_min, localtime.tm_sec, now.tv_usec / 1000,
		getpid());
	vsnprintf(buff+n, sizeof(buff)-n-1, fmt, ap);
	fprintf(fp, buff);
	sd_va_end(ap);
	fflush(fp);

#if defined(_DUOKAN)
	ANDROID_LOG( ANDROID_LOG_DEBUG, "xunlei_running_log: ", "%s", buff);
#endif
	return 0;
}
#endif //_RELEASE_LOG





#ifdef _LOGGER
//#ifdef AMLOS
#if defined(AMLOS) && !defined(MEDIA_CENTER_CUSTOMER_LOG)
extern _int32 write_log ( _int32 f_urt , unsigned char * buffer, _u32 write_length);
_int32  g_amlos_log = 2;
#endif
_int32 logger(_int32 logid, const char *fmt, sd_va_list ap)
{
#if defined(LINUX) && !defined(MEDIA_CENTER_CUSTOMER_LOG) 

	struct tm localtime;
	struct timeval now;
	static BOOL check_flag = FALSE;
	/* 由于sd_task_lock 在多线程同时等待同一把锁的时候有问题，暂时一个时间只能打一个线程的log! */
	//if(check_flag) return SUCCESS;
	sd_task_lock(log_lock());
	check_flag = TRUE;
	gettimeofday(&now, NULL);
	localtime_r(&now.tv_sec, &localtime);

	fprintf(log_file(), "\n[%d-%d-%d %d:%d:%d:%ld %s:%u]: ", 
		localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
		localtime.tm_hour, localtime.tm_min, localtime.tm_sec, now.tv_usec / 1000,
		get_logdesc(logid), getpid()/*sd_get_self_taskid()*/);

	vfprintf(log_file(), fmt, ap);

#if defined(_ANDROID_ARM)
	if(lseek(fileno(log_file()), 0, SEEK_CUR) >= log_max_filesize()) {
		lseek(fileno(log_file()), 0, SEEK_SET);
	}		
#endif

	check_flag = FALSE;
	sd_task_unlock(log_lock());
	
#elif defined(AMLOS) && !defined(MEDIA_CENTER_CUSTOMER_LOG)

	sd_task_lock(log_lock());

	_int32 log_fd = g_amlos_log;
	struct tm *r_time = NULL;
	struct timeval sunow;

	gettimeofday(&sunow, NULL);
	r_time = localtime(&sunow.tv_sec);

	int ret_val = SUCCESS;

       //log_fd = open(log_file(), O_RDWR|O_APPEND, 0);
	char szLogBuf[256];
	sd_snprintf(szLogBuf, sizeof(szLogBuf), "[%d-%d-%d %d:%d:%d:%ld", r_time->tm_year + 1900, r_time->tm_mon + 1, r_time->tm_mday,
		r_time->tm_hour, r_time->tm_min, r_time->tm_sec, sunow.tv_usec / 1000);
	//ret_val = write(log_fd, szLogBuf, sd_strlen(szLogBuf));
	write_log (log_fd, (unsigned char*)szLogBuf, (_u32)sd_strlen(szLogBuf));

	sd_snprintf(szLogBuf, sizeof(szLogBuf), " %s:%u]: ", get_logdesc(logid), sd_get_self_taskid());
	//ret_val = write(log_fd, szLogBuf, sd_strlen(szLogBuf));
	write_log (log_fd, (unsigned char*)szLogBuf, (_u32)sd_strlen(szLogBuf));

	sd_vsnprintf(szLogBuf, sizeof(szLogBuf), fmt, ap);
	//ret_val = write(log_fd, szLogBuf, sd_strlen(szLogBuf));
	write_log (log_fd, (unsigned char*)szLogBuf, (_u32)sd_strlen(szLogBuf));

	sd_snprintf(szLogBuf, sizeof(szLogBuf), "%s", "\n");
	//ret_val = write(log_fd, szLogBuf, sd_strlen(szLogBuf));
	write_log (log_fd, (unsigned char*)szLogBuf, (_u32)sd_strlen(szLogBuf));

	check_logfile_size(log_fd);

	//ret_val = close(log_fd);	

	sd_task_unlock(log_lock());
	
#elif defined(__SYMBIAN32__)

	_int32 ret = 0, len = 0;
	_u32 writesize = 0;
	_int64 now_time = 0;
	_u32 year, mon, day, hour, min, sec, msec;
	_u64 file_size = 0;
	static BOOL check_flag = FALSE;
	static char szLogBuf[2048] = {0};
	/* 由于sd_task_lock 在多线程同时等待同一把锁的时候有问题，暂时一个时间只能打一个线程的log! */
#ifndef __GCCE__
	if(check_flag) return SUCCESS;
#endif
	sd_task_lock(log_lock());
	check_flag = TRUE;
	ret = sd_symbian_time_get_current_time_info(&now_time, &year, &mon, &day, &hour, &min, &sec, &msec, NULL, NULL, NULL);
	sd_snprintf(szLogBuf + len, sizeof(szLogBuf) - len, "[%d-%d-%d %d:%d:%d:%ld", year, mon, day,
		hour, min, sec, msec);
	len += sd_strlen(szLogBuf + len);

	sd_snprintf(szLogBuf + len, sizeof(szLogBuf) - len, " %s:%u]: ", get_logdesc(logid), sd_get_self_taskid());
	len += sd_strlen(szLogBuf + len);

	sd_vsnprintf(szLogBuf + len, sizeof(szLogBuf) - len, fmt, ap);
	len += sd_strlen(szLogBuf + len);

	sd_snprintf(szLogBuf + len, sizeof(szLogBuf) - len, "%s", "\n");
	len += sd_strlen(szLogBuf + len);

	ret = sd_write(get_log_fd(), szLogBuf, sd_strlen(szLogBuf), &writesize);
	check_logfile_size(get_log_fd());
	check_flag = FALSE;
	sd_task_unlock(log_lock());
		
#elif defined(MSTAR)

	sd_task_lock(log_lock());
	
	_u32 now = MsOS_GetSystemTime(); //开机启动到现在的毫秒数

	fprintf(log_file(), "[%d", now);

	fprintf(log_file(), " %s:%u]: ", get_logdesc(logid), sd_get_self_taskid());

	vfprintf(log_file(), fmt, ap);
	fprintf(log_file(), "\n");

	fflush(log_file());

//	check_logfile_size();


	sd_task_unlock(log_lock());

	return SUCCESS;

#elif defined(WINCE)
   TIME_t now;
	unsigned int millitm1;
	_int32 ret_val = SUCCESS;
	
	sd_task_lock(log_lock());

	ret_val = sd_local_time(&now);
	CHECK_VALUE(ret_val);
	
	ret_val = sd_time_ms(&millitm1);
	CHECK_VALUE(ret_val);
	millitm1 = millitm1%1000;
	fprintf(log_file(), "[%d-%d-%d %02d:%02d:%02d:%ld", 
		now.year, now.mon + 1, now.mday,
		now.hour, now.min, now.sec, millitm1 );
	
	fprintf(log_file(), " %s:%u]: ", get_logdesc(logid), sd_get_self_taskid());
	vfprintf(log_file(), fmt, ap);
	fprintf(log_file(), "\n");

	check_logfile_size();

	sd_task_unlock(log_lock());
#elif defined(SUNPLUS) || defined(MEDIA_CENTER_CUSTOMER_LOG)



	struct tm localtime;
	struct timeval now;
	time_t  now_time;
	static char write_buffer[2048];
	_u32  written_size;


	sd_task_lock(log_lock());

	gettimeofday(&now, NULL);
	time(&now_time);
	localtime_r(&now_time, &localtime);
	

	if(log_file() != -1)
	{

	      sd_memset(write_buffer, 0, sizeof(write_buffer));
	      sd_snprintf(write_buffer , sizeof(write_buffer)-1,"[%d-%d-%d %d:%d:%d:%d", 
			localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
			localtime.tm_hour, localtime.tm_min, localtime.tm_sec, now.tv_usec / 1000);
	      if(is_available_ci(CI_LOG_WRITE_LOG))
	      {
			((et_log_write_log)ci_ptr(CI_LOG_WRITE_LOG))( log_file(), write_buffer, sd_strlen(write_buffer), &written_size);
	      }
	      else
	      {
	          sd_write(log_file(), write_buffer, sd_strlen(write_buffer), &written_size);
	      	}

	      sd_memset(write_buffer, 0, sizeof(write_buffer));
	      sd_snprintf(write_buffer , sizeof(write_buffer)-1, "%s:%u]:" ,get_logdesc(logid), sd_get_self_taskid());
	      if(is_available_ci(CI_LOG_WRITE_LOG))
	      {
			((et_log_write_log)ci_ptr(CI_LOG_WRITE_LOG))( log_file(), write_buffer, sd_strlen(write_buffer), &written_size);
	      }
	      else
	      {
	          sd_write(log_file(), write_buffer, sd_strlen(write_buffer), &written_size);
	      	}

	      sd_memset(write_buffer, 0, sizeof(write_buffer));
	      sd_vsnprintf(write_buffer , sizeof(write_buffer)-1,fmt, ap);
	      if(is_available_ci(CI_LOG_WRITE_LOG))
	      {
			((et_log_write_log)ci_ptr(CI_LOG_WRITE_LOG))( log_file(), write_buffer, sd_strlen(write_buffer), &written_size);
	      }
	      else
	      {
	          sd_write(log_file(), write_buffer, sd_strlen(write_buffer), &written_size);
	      	}

	      sd_memset(write_buffer, 0, sizeof(write_buffer));
	      sd_snprintf(write_buffer , sizeof(write_buffer)-1, "\n");
	      if(is_available_ci(CI_LOG_WRITE_LOG))
	      {
			((et_log_write_log)ci_ptr(CI_LOG_WRITE_LOG))( log_file(), write_buffer, sd_strlen(write_buffer), &written_size);
	      }
	      else
	      {
	          sd_write(log_file(), write_buffer, sd_strlen(write_buffer), &written_size);
	      	}

	      check_logfile_size();
	}
	else
	{
	fprintf(stdout, "[%d-%d-%d %02d:%02d:%02d:%ld", 
		localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
		localtime.tm_hour, localtime.tm_min, localtime.tm_sec, now.tv_usec / 1000);
	/*sd_fprintf(fileno(log_file()), "[%d-%d-%d %d:%d:%d:%ld", 
		localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
		localtime.tm_hour, localtime.tm_min, localtime.tm_sec, now.tv_usec / 1000);
*/
	fprintf(stdout, " %s:%u]: ", get_logdesc(logid), sd_get_self_taskid());
	fprintf(stdout, fmt, ap);
	fprintf(stdout, "\n");
	}


	sd_task_unlock(log_lock());
#endif
	return SUCCESS;
}


#endif

static char g_urgent_file_path[MAX_FILE_NAME_BUFFER_LEN] = {0};

_int32 set_urgent_file_path(const char * path)
{
	sd_strncpy(g_urgent_file_path, path, MAX_FILE_NAME_BUFFER_LEN-1);
	if(g_urgent_file_path[sd_strlen(g_urgent_file_path)-1]!=DIR_SPLIT_CHAR)
	{
		char * pos = sd_strrchr(g_urgent_file_path, DIR_SPLIT_CHAR);
		if(pos!=NULL)
		{
			*(pos+1) = '\0';
		}
		else
		{
			g_urgent_file_path[0] = '.';
			g_urgent_file_path[1] = DIR_SPLIT_CHAR;
			g_urgent_file_path[2] = '\0';
		}
	}
	sd_strcat(g_urgent_file_path,"de_urgent_file.txt",sd_strlen("de_urgent_file.txt"));
	return SUCCESS;
}

_int32 print_strace_to_file(void)
{
#if 0 //defined(_PC_LINUX)
#if (!defined(ENABLE_ETM_MEDIA_CENTER)) 
	_int32 ret_val = SUCCESS;
	_u32 file_id = 0;
	_u64 filesize = 0;
	if(sd_strlen(g_urgent_file_path)==0) return -1;

	ret_val = sd_open_ex(g_urgent_file_path, O_FS_RDWR|O_FS_CREATE,&file_id);
	if(ret_val!=SUCCESS) return ret_val;

	sd_filesize( file_id, &filesize);
	sd_setfilepos( file_id, filesize);
		

	/*  print_strace */
	{
	        void *array[20] = {0};
	        size_t size;
	        char **strings;
	        size_t i;
	        int nCount = 0;

	        size =backtrace(array,20);
	        strings = backtrace_symbols(array,size);
		sd_fprintf(file_id,"***************print_strace start ********************\n");
	        for(i = 1;i<size;i++)
	        {
			sd_fprintf(file_id,"%s\n",strings[i]);
			#if 0 //def _DEBUG
	                printf("%s\n",strings[i]);
			#endif
	                if(sd_strstr(strings[i],"ILibWebServer_Release",0)!=NULL)
	                {
	                        nCount++;
	                }
	        }
		sd_fprintf(file_id,"***************print_strace end!********************\n\n");
	        free(strings);
	}
	
	sd_close_ex( file_id);
#endif
#endif
	return SUCCESS;
}	

_int32 write_urgent_to_file_impl(const char *fmt, sd_va_list ap)
{
#if 0 //defined(LINUX) 
	_int32 ret_val = SUCCESS;
	_u32 file_id = 0;
	_u64 filesize = 0;
	struct tm local_time;
	struct timeval now;

	gettimeofday(&now, NULL);
	localtime_r(&now.tv_sec, &local_time);

	if(sd_strlen(g_urgent_file_path)==0) return -1;

	ret_val = sd_open_ex(g_urgent_file_path, O_FS_RDWR|O_FS_CREATE,&file_id);
	if(ret_val!=SUCCESS) return ret_val;

	sd_filesize( file_id, &filesize);
	sd_setfilepos( file_id, filesize);
		
	sd_fprintf(file_id, "[%d-%d-%d %d:%d:%d:%ld", 
		local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday,
		local_time.tm_hour, local_time.tm_min, local_time.tm_sec, now.tv_usec / 1000);
	
#ifdef _ANDROID_LINUX
	sd_fprintf(file_id, ",tid:%u]: ",  /* getpid() */ sd_get_self_taskid());
#else
	sd_fprintf(file_id, ",tid:%u]: ", getpid()/*sd_get_self_taskid()*/);
#endif
	sd_vfprintf(file_id, fmt, ap);
	sd_fprintf(file_id, "\n");

	sd_close_ex( file_id);
#endif
	return SUCCESS;
}	

_int32  write_urgent_to_file(const char *fmt, ...)
{
	_int32 ret_val = SUCCESS;
#if 0 // defined(LINUX)
	sd_va_list ap;
	sd_va_start(ap, fmt);
	ret_val = write_urgent_to_file_impl( fmt, ap);
	sd_va_end(ap);
#else
	sd_assert(FALSE);
#endif
	return ret_val;
}

