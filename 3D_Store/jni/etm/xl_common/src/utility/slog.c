#include "utility/slog.h"
#include "utility/define.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include "utility/logid.h"
#include "platform/sd_fs.h"

#include "platform/android_interface/sd_android_log.h"

#ifdef _LOGGER

#define BUFSIZE 1024*2

typedef enum __log_level
{
    SLOG_LEVEL_TRACE = 0,
	SLOG_LEVEL_DEBUG,
	SLOG_LEVEL_INFO,
	SLOG_LEVEL_WARN,
	SLOG_LEVEL_ERROR,
    SLOG_LEVEL_NOLOG
}LOG_LEVEL;

typedef struct _data_node_
{
	struct _data_node_* next;
	struct _data_node_* prev;
	int str_len;
}DataNode;

//configure sample
/*
 * ### log级别
 * slog_level=DEBUG
 * ### log文件名
 * slog_log_name=../log/server.log
 * ### log 文件最大大小(单位M)
 * slog_log_maxsize=10M
 * ### log文件最多个数
 * slog_log_maxcount=10
 * ### log缓冲区大小(单位KB,默认512KB)
 * slog_flush_size=1024
 * ### log缓冲刷新间隔(单位s,默认1s)
 * slog_flush_interval=2
 * ### log动态更新配置参数的时间间隔(单位s,默认60s)
 * config_update_interval=30
*/
typedef struct _slog_setting_
{
	char config_name[128];  //配置文件名
#if defined(MACOS)&&defined(MOBILE_PHONE)
	char log_name[MAX_FULL_PATH_BUFFER_LEN];     //输出文件名
#else
	char log_name[128];     //输出文件名
#endif
	LOG_LEVEL log_level;    //log的级别(默认DEBUG)
	FILE* log_file;         //当前打开到log文件(默认stdout)
	int log_size;           //当前log文件大小
	pthread_t thread_id;    //刷新线程
	int flush_size;         //当缓存大小超过该值时进行刷新log的缓存到slog_file(单位KB,默认512KB)
	int flush_interval;     //刷新log的缓存到slog_file的时间间隔(单位s,默认1s)
	int config_update_interval; //更新配置文件的时间间隔(单位s,默认60s)

	//当slog_file为stdout时以下3个字段无效
	int log_count;          //当前最大文件索引号
	int log_max_size;       //每个log数据文件到最大大小(单位M,默认10M)
	int log_max_count;      //log文件到个数(默认10个)

	int buf_size;           //当前缓冲区大小
	DataNode buf_list;      //缓冲链表

	pthread_mutex_t mutex;
	pthread_mutex_t cond_mutex;
	pthread_cond_t cond;
	int exit_flush_thread;
}SlogSetting;
static SlogSetting g_slog_setting;

//创建刷新线程
static void create_flush_thread();
//刷新线程
static void* flush_thread(void* user_data);
//刷新log缓冲区到输出文件
static void flush_buffer_to_file();

//打印配置参数
static void print_config();
//打印时间格式
static void format_time(char *buf, int bufLen);
//格式化数据
static void format_to_string(char *str, int str_size, const char *log_level, const char* file, const int line, const char *func, int logid, const char* fmt, va_list* args);
//将临时数据输出到log的缓冲区
static void output_string_to_buffer(const char *str);
//通知线程刷新缓冲区
static void notify_flush();	//通知刷新线程
//当log文件满时,移动log文件
static void rename_log_file();

static unsigned int get_thread_id(void);

static int sLocalSlogNeedInitialized = 1;

static BOOL s_log_flush_thread_exit_succ = FALSE;

#if defined(MACOS)&&defined(MOBILE_PHONE)
int slog_init(const char* config,const char* etm_system_path,unsigned int path_len)
#else
int slog_init(const char* config)
#endif
{
	if(sLocalSlogNeedInitialized)
	{
		sLocalSlogNeedInitialized = 0;
		g_slog_setting.config_name[0]           = '\0';
		g_slog_setting.log_name[0]              = '\0';
		g_slog_setting.log_level                = SLOG_LEVEL_DEBUG;
		g_slog_setting.log_file                 = stdout;
		g_slog_setting.log_size                 = 0;
		g_slog_setting.flush_size               = 512*1024;	//默认512KB
		g_slog_setting.flush_interval           = 1;		//默认刷新间隔1s
		g_slog_setting.config_update_interval   = 60;		//默认60s
		g_slog_setting.buf_size                 = 0;		//slog缓冲区大小
		g_slog_setting.buf_list.next            = NULL;		//slog缓冲区链表
		g_slog_setting.buf_list.prev            = NULL;		//slog缓冲区链表
		//the 3 elements are invalid when slog_file is set to stdout
		g_slog_setting.log_max_size             = 10;		//10M
		g_slog_setting.log_max_count            = 10;		//10 log files
		g_slog_setting.log_count                = 0;		//当前生成的log文件数

		g_slog_setting.exit_flush_thread        = 0;
		pthread_mutex_init(&g_slog_setting.mutex, NULL);
		pthread_mutex_init(&g_slog_setting.cond_mutex, NULL);
		pthread_cond_init(&g_slog_setting.cond, NULL);
		create_flush_thread();
	}

	if(config==NULL || strlen(config)==0)
	{
		fprintf(stderr, "WARN!!! slog config file is invalid !!!\n");
		print_config();
		return 0;
	}

	pthread_mutex_lock(&g_slog_setting.mutex);  //刷新线程会调用SLOG_INIT函数来更新配置参数

	char buf[1024];
	FILE* file = NULL;
	if((file = fopen(config, "r"))==NULL)
	{
		pthread_mutex_unlock(&g_slog_setting.mutex);
		fprintf(stderr, "can't open slog config file:%s !!!\n", config);
		return 0;
	}
#if !(defined(MACOS)&&defined(MOBILE_PHONE))
	//save the configure name
	if(g_slog_setting.config_name[0] == '\0')
		strcpy(g_slog_setting.config_name, config);
#endif
	logger_printf("loading %s...\n", config);
	//parse configure file
	while(fgets(buf,1024,file) != NULL)  //read one line
	{
		int i=0, len=strlen(buf);
		while(i<len && (buf[i]==' ' || buf[i]=='\t'))
			++i;
		if(buf[i]=='\n' || buf[i]=='\r' || buf[i]=='#' || buf[i]=='/')	//属于空行或者注释
			continue;

		//get key and value
		char *key = strtok(&buf[i], "=");
		if(key == NULL)
			continue;
		char *value = strtok(NULL, "=");
		if(value == NULL)
			continue;
		//parse key and value
		if(strcmp(key, "slog_level") == 0)
		{
			char str[30];
			sscanf(value, "%s", str);
			if(strcmp(str, "TRACE")==0)
				g_slog_setting.log_level = SLOG_LEVEL_TRACE;
			else if(strcmp(str, "DEBUG")==0)
				g_slog_setting.log_level = SLOG_LEVEL_DEBUG;
			else if(strcmp(str, "INFO")==0)
				g_slog_setting.log_level = SLOG_LEVEL_INFO;
			else if(strcmp(str, "WARN")==0)
				g_slog_setting.log_level = SLOG_LEVEL_WARN;
			else if(strcmp(str, "ERROR")==0)
				g_slog_setting.log_level = SLOG_LEVEL_ERROR;
		}
		else if(strcmp(key, "slog_log_name") == 0)
		{
			if(g_slog_setting.log_name[0] != '\0')
				continue;

#if defined(MACOS)&&defined(MOBILE_PHONE)
            char log_filename[MAX_FULL_PATH_BUFFER_LEN];
            sd_memset(log_filename,0x00,MAX_FULL_PATH_BUFFER_LEN);
            if((etm_system_path!=NULL)&&(path_len!=0))
            {
                sd_strncpy(log_filename,etm_system_path,path_len);
                if(log_filename[path_len-1]!='/') sd_strcat(log_filename,"/",1);
            }
            sd_strcat(log_filename,value,sd_strlen(value));
            
#else
			char log_filename[128];
			sscanf(value, "%s", log_filename);
#endif

#ifdef _ANDROID_LINUX
			_u32 ts = 0;
			sd_time(&ts);
   		    memset(log_filename, 0, sizeof(log_filename));
			sprintf(log_filename, "/sdcard/server.%u.log", ts);
#endif
                    
			g_slog_setting.log_file = fopen(log_filename, "w");
			if(g_slog_setting.log_file != NULL)
			{
				strcpy(g_slog_setting.log_name, log_filename);
			}
			else
			{
				g_slog_setting.log_file = stdout;
				fprintf(stderr, "ERROR!!! open log file=%s failed.use stdout.", log_filename);
			}
		}
		else if(strcmp(key, "slog_log_maxsize") == 0)
		{
			sscanf(value, "%d", &g_slog_setting.log_max_size);
		}
		else if(strcmp(key, "slog_log_maxcount") == 0)
		{
			sscanf(value, "%d", &g_slog_setting.log_max_count);
		}
		else if(strcmp(key, "slog_flush_size") == 0)
		{
			sscanf(value, "%d", &g_slog_setting.flush_size);
			g_slog_setting.flush_size *= 1024;
		}
		else if(strcmp(key, "slog_flush_interval") == 0)
		{
			sscanf(value, "%d", &g_slog_setting.flush_interval);
		}
		else if(strcmp(key, "config_update_interval") == 0)
		{
			sscanf(value, "%d", &g_slog_setting.config_update_interval);
		}
	}//while
    
    sd_fclose(file);

	sd_log_reload(config);
	pthread_mutex_unlock(&g_slog_setting.mutex);
    print_config();
	return 0;
}

void slog_finalize()
{
    int cnt = 0;
	char str[BUFSIZE];
    format_time(str, BUFSIZE);
    sprintf(str+strlen(str), "[SLOG UNINIT]...");
    output_string_to_buffer(str);

    g_slog_setting.exit_flush_thread = 1;
    pthread_join(g_slog_setting.thread_id, NULL);
    //printf("slog flush thread exit.\n");


#ifdef _LOGGER
    while(!s_log_flush_thread_exit_succ && ++cnt < 10000)
    {
        sd_sleep(20);
    }
#else
    while(!s_log_flush_thread_exit_succ && ++cnt < 100)
    {
        sd_sleep(20);
    }
#endif


    if(g_slog_setting.log_file!=NULL && g_slog_setting.log_file!=stdout)
    {
    	fclose(g_slog_setting.log_file);
    	g_slog_setting.log_file = stdout;
    }
    pthread_mutex_destroy(&g_slog_setting.mutex);
    pthread_mutex_destroy(&g_slog_setting.cond_mutex);
    pthread_cond_destroy(&g_slog_setting.cond);

    sLocalSlogNeedInitialized = 1;
}

void slog_trace(const char* file, const int line, const char *func, int logid, const char* fmt, ...)
{
    if(SLOG_LEVEL_TRACE >= g_slog_setting.log_level)
    {
	char str[BUFSIZE];
	int len = sizeof(str);
	va_list args;
	va_start(args, fmt);
		format_to_string(str, BUFSIZE, "TRACE", file, line, func, logid, fmt, &args);
	va_end(args);
	ANDROID_LOG(ANDROID_LOG_DEBUG, "xldpengine", "%s", str);
		output_string_to_buffer(str);
    }
}

void slog_debug(const char* file, const int line, const char *func, int logid, const char* fmt, ...)
{
    if(SLOG_LEVEL_DEBUG >= g_slog_setting.log_level)
    {
		char str[BUFSIZE];
		int len = sizeof(str);
		va_list args;
		va_start(args, fmt);
		format_to_string(str, BUFSIZE, "DEBUG", file, line, func, logid, fmt, &args);
		va_end(args);
		ANDROID_LOG(ANDROID_LOG_DEBUG, "xldpengine", "%s", str);
		output_string_to_buffer(str);
    }
}

void slog_info(const char* file, const int line, const char *func, int logid, const char* fmt,  ...)
{
    if(SLOG_LEVEL_INFO >= g_slog_setting.log_level)
    {
		char str[BUFSIZE];
		va_list args;
		//格式化
		va_start(args, fmt);
		format_to_string(str, BUFSIZE, "INFO", file, line, func, logid, fmt, &args);
		va_end(args);
		//输出
		ANDROID_LOG(ANDROID_LOG_INFO, "xldpengine", "%s", str);
		output_string_to_buffer(str);
    }
}
void slog_warn(const char* file, const int line, const char *func, int logid, const char* fmt,  ...)
{
    if(SLOG_LEVEL_WARN >= g_slog_setting.log_level)
    {
		char str[BUFSIZE];
		va_list args;
		//格式化
		va_start(args, fmt);
		format_to_string(str, BUFSIZE, "WARN", file, line, func, logid, fmt, &args);
		va_end(args);
		//输出
		ANDROID_LOG(ANDROID_LOG_WARN, "xldpengine", "%s", str);
		output_string_to_buffer(str);
    }
}

void slog_error(const char* file, const int line, const char *func, int logid, const char* fmt, ...)
{
    if(SLOG_LEVEL_ERROR >= g_slog_setting.log_level)
    {
		char str[BUFSIZE];
		va_list args;
		//格式化
		va_start(args, fmt);
		format_to_string(str, BUFSIZE, "ERROR", file, line, func, logid, fmt, &args);
		va_end(args);
		//输出
		ANDROID_LOG(ANDROID_LOG_ERROR, "xldpengine", "%s", str);
		output_string_to_buffer(str);
    }
}

void slog_urgent(const char* file, const int line, const char *func, int logid, const char *fmt, ...)
{
    if(SLOG_LEVEL_ERROR >= g_slog_setting.log_level)
    {
		char str[BUFSIZE];
		va_list args;
		//格式化
		va_start(args, fmt);
		format_to_string(str, BUFSIZE, "ERROR", file, line, func, logid, fmt, &args);
		va_end(args);
		//输出
		ANDROID_LOG(ANDROID_LOG_ERROR, "xldpengine", "%s", str);
		output_string_to_buffer(str);
    }
}

/////////////////////////////  static function  /////////////////////////////////
//格式化时间
void format_time(char *buf, int bufLen)
{
#if 0
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
	sprintf(buf,"%d-%02d-%02d %02d:%02d:%02d",t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
#elif (defined(MACOS)&&defined(MOBILE_PHONE))//ios //zion 2013-01-11
    //snprintf(buf, bufLen, "%04d-%02d-%02d %02d:%02d:%02d:%03d",1,2,3,4,5,6,7);
    long ts=time(NULL);
    snprintf(buf, bufLen, "--%ld--",ts%100000);
 

#else
	struct tm local_time;
	struct timeval now;

	gettimeofday(&now, NULL);
	localtime_r(&now.tv_sec, &local_time);

	snprintf(buf, bufLen, "%04d-%02d-%02d %02d:%02d:%02d:%03ld",
	local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday,
	local_time.tm_hour, local_time.tm_min, local_time.tm_sec, now.tv_usec / 1000);

#endif
}

//打印当前配置参数
void print_config()
{
	char str[512];
	format_time(str, 512);
	sprintf(str+strlen(str),"[SLOG INIT]current config of SLOG"
			":log_level=%d"
			",log_file=%s"
			",max_log_size=%d(M)"
			",max_log_count=%d"
			",flush_size=%d"
			",flush_interval=%d"
			",config_update_interval=%d."
			,g_slog_setting.log_level
			,g_slog_setting.log_name[0]!='\0'?g_slog_setting.log_name:"stdout"
			,g_slog_setting.log_max_size
			,g_slog_setting.log_max_count
			,g_slog_setting.flush_size
			,g_slog_setting.flush_interval
			,g_slog_setting.config_update_interval);

	output_string_to_buffer(str);
}

static unsigned int get_thread_id(void)
{
#if defined(LINUX)	
	return pthread_self();
#elif defined(WINCE)
	return GetCurrentThreadId();
#endif
	return 0;
}

#define TIME_STR_LEN 23
//格式化输入内容到buf
void format_to_string(char *str, int str_size, const char *log_level, const char* file, const int line, const char *func, int logid, const char* fmt, va_list* args)
{
	int len;
	if(str==NULL || str_size<100)  //打印时间格式需要预留空间
		return ;

    //注意，为了减少一次strlen调用，时间格式固定长度为23，例如：2013-01-06 15:36:38:993
	format_time(str, TIME_STR_LEN+1);
	//if(log_level != NULL)
	//	sprintf(str+23, " [%s] [%s] [%u] ", log_level, get_logdesc(logid), get_thread_id() );
	if(log_level != NULL)
		snprintf(str+TIME_STR_LEN, str_size-TIME_STR_LEN, " [%u] %s [%s] [%s:%d] ", get_thread_id(), log_level, get_logdesc(logid), func, line );
	len = strnlen(str, str_size);
	if (str_size > len)
	{
		vsnprintf(str+len, str_size-len, fmt, *args);
	} 
	else
	{
		str[str_size-1] = '\0';
	}
}

//输出buf的内容(以'\0'结束)
void output_string_to_buffer(const char *str)
{
	//  如果已经反初始化或者已经退出线程，则不缓存所有打印
	if (sLocalSlogNeedInitialized || g_slog_setting.exit_flush_thread) 
	{
		return;
	}
	
	//判断线程是否已经挂掉
	int kill_result = pthread_kill(g_slog_setting.thread_id, 0);
	if(kill_result == ESRCH)
		create_flush_thread();

	int len = strlen(str)+2;  //include '\n' and '\0'
	char* buf = calloc(sizeof(DataNode)+len, 1);
	DataNode* data_node = (DataNode*)buf;
	data_node->str_len = len-1; //exclude '\0'
	buf += sizeof(DataNode);
	strcpy(buf, str);
	buf[data_node->str_len-1] = '\n';

	//将数据插入到链表头(逆序)
	pthread_mutex_lock(&g_slog_setting.mutex);
	data_node->prev = NULL;
	data_node->next = g_slog_setting.buf_list.next;
	g_slog_setting.buf_list.next = data_node;
	if(data_node->next != NULL)
		data_node->next->prev = data_node;
	if(g_slog_setting.buf_list.prev == NULL)
		g_slog_setting.buf_list.prev = data_node;

	g_slog_setting.buf_size += len;
	if(g_slog_setting.buf_size >= g_slog_setting.flush_size)
		notify_flush();	//通知线程刷新缓冲区
	pthread_mutex_unlock(&g_slog_setting.mutex);
	return;
}

void notify_flush()
{
	//printf("notify_flush\n");
	pthread_mutex_lock(&g_slog_setting.cond_mutex);
	pthread_cond_signal(&g_slog_setting.cond);
	pthread_mutex_unlock(&g_slog_setting.cond_mutex);
	return ;
}

//slog的刷新线程
void* flush_thread(void* user_data)
{
	struct timeval start, last_update;
	struct timespec timeout;

	last_update.tv_sec = 0;
	while(g_slog_setting.exit_flush_thread != 1)
	{
		gettimeofday(&start, NULL);
		if(last_update.tv_sec == 0)
			last_update.tv_sec = start.tv_sec;

		timeout.tv_sec = start.tv_sec + g_slog_setting.flush_interval;
		timeout.tv_nsec = start.tv_usec*1000;
		//等待超时或者刷新通知
		pthread_mutex_lock(&g_slog_setting.cond_mutex);
		pthread_cond_timedwait(&g_slog_setting.cond, &g_slog_setting.cond_mutex, &timeout);
		pthread_mutex_unlock(&g_slog_setting.cond_mutex);
		//将缓冲区数据刷新到log文件
		flush_buffer_to_file();
        
		gettimeofday(&start, NULL);
		if(start.tv_sec-last_update.tv_sec > g_slog_setting.config_update_interval)
		{
			printf("init...\n");
#if defined(MACOS)&&defined(MOBILE_PHONE)
                  slog_init(g_slog_setting.config_name, NULL, 0);
#else
		    slog_init(g_slog_setting.config_name);  //update the configure at short intervals
#endif
		    last_update.tv_sec = start.tv_sec;
		}
	}
	//将缓冲区的剩余数据刷新到log文件
	flush_buffer_to_file();

    s_log_flush_thread_exit_succ = TRUE;

	return NULL;
}

void create_flush_thread()
{
	if(pthread_create(&g_slog_setting.thread_id, NULL, flush_thread, (void*)NULL) != 0)
	{
		fprintf(g_slog_setting.log_file, "ERROR!!! create flush thread failed. exit !!!");
		exit(1);
	}
}

void flush_buffer_to_file()
{
	//printf("flush to file\n");
	DataNode *temp = NULL, *data_list = NULL;
	pthread_mutex_lock(&g_slog_setting.mutex);
	data_list = g_slog_setting.buf_list.prev;
	g_slog_setting.buf_list.next = g_slog_setting.buf_list.prev = NULL;
	g_slog_setting.buf_size = 0;
	pthread_mutex_unlock(&g_slog_setting.mutex);

	int log_size = g_slog_setting.log_max_size*1024*1024;
	for(temp=data_list; temp && g_slog_setting.log_file != NULL; temp=data_list)
	{
		data_list = temp->prev;
		//fprintf(g_slog_setting.log_file, "%s", (char*)temp+sizeof(DataNode));
		fwrite((char*)temp+sizeof(DataNode), temp->str_len, 1, g_slog_setting.log_file);

		g_slog_setting.log_size += temp->str_len; //include '\n'
		free(temp);
        temp = NULL;
		if(g_slog_setting.log_size >= log_size) //重新创建新的log文件
			rename_log_file();
	}
}

void rename_log_file()
{
	//printf("rename log file\n");
	g_slog_setting.log_size = 0;
	if(g_slog_setting.log_name[0] == '\0' || g_slog_setting.log_file==stdout)
		return ;
	fclose(g_slog_setting.log_file);

	int i = g_slog_setting.log_count;
	char old_log_name[128],new_log_name[128];
	while(i > 0)
	{
		snprintf(old_log_name, 128, "%s.%d", g_slog_setting.log_name, i);
		snprintf(new_log_name, 128, "%s.%d", g_slog_setting.log_name, i+1);
		if(i == g_slog_setting.log_max_count)
			remove(old_log_name);
		else
			rename(old_log_name, new_log_name);
		--i;
	}

	snprintf(old_log_name, 128, "%s", g_slog_setting.log_name);
	snprintf(new_log_name, 128, "%s.%d", g_slog_setting.log_name, i+1);
	rename(old_log_name, new_log_name);

	if(g_slog_setting.log_count < g_slog_setting.log_max_count)
		++g_slog_setting.log_count;

	g_slog_setting.log_file = fopen(old_log_name, "w");
	if(g_slog_setting.log_file == NULL)
	{
		g_slog_setting.log_file = stdout;
		fprintf(stderr, "ERROR!!! open log file=%s failed.use stdout.", old_log_name);
	}
}

#endif
