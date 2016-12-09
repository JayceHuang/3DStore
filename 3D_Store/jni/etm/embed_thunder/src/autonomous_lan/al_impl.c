#ifdef AUTO_LAN
#include "utility/define.h"
#include "utility/thread.h"
#include "utility/settings.h"
#include "utility/peerid.h"
#include "utility/local_ip.h"
#include "utility/utility.h"
#include "platform/sd_fs.h"

#include "utility/logid.h"
#define LOGID LOGID_AUTO_LAN
#include "utility/logger.h"

#include "al_impl.h"
#include "al_trace_route.h"
#include "al_ping.h"
#include "socket_proxy_interface.h"


/*  network condition      */
#define	AL_NET_DISCONNECT (-2)
#define	AL_NET_BAD (-1)
#define	AL_NET_NORMAL (0)
#define	AL_NET_GOOD (1)


static THREAD_STATUS	g_al_thread_status = INIT;
static _int32			g_al_thread_id = 0;
static _int32			g_ping_delay_times = 0;			//五次ping的延时之和
static _int32			g_max_ping_delay_time = 0;	//五次ping中最大的延时	
static _int32			g_min_ping_delay_time = PING_TIMEOUT;		// 五次ping中最小的延时
static _int32			g_average_delay_time = 0;		//五次ping的时延去掉最大和最小值之后的平均值
static _int32			g_net_warning_flag = 0;			//网络状况预警标志: -1为差,0为正常,1为好
static _u32			g_speed_up_per = 20;			//加速步进
static _u32			g_speed_down_per = 10;			//减速步进
static _u32			g_max_ping_time = 300;			//判定网速为坏的ping时延阀值
static _u32			g_min_ping_time = 100;			//判定网速为好的ping时延阀值
static _u32			g_schedule_timeout = 1;			//速度控制调度时间间隔,单位:秒
static BOOL			g_is_auto_lan_enable = FALSE;
static char 			g_physical_addr[16];
static _u32			g_local_ip = 0;
static _u32			g_check_ip_count = 0;
static _u32			g_download_limit_speed=-1;
static _u32			g_upload_limit_speed=-1;

#ifdef AL_DEBUG
static THREAD_STATUS	g_al_test_thread_status = INIT;
static _int32			g_al_test_thread_id = 0;
void al_test_thread(void* param);
static _u32			g_ping_ip = 0;
#endif

//上传速度值
#define		UPLOAD_SPEED_MIN		10
#define		UPLOAD_SPEED_BASE	200			//基本上传速度200K
#define		UPLOAD_SPEED_MAX		2048		//最大上传速度2M
//下载速度值
#define		DOWNLOAD_SPEED_MIN	20
#define		DOWNLOAD_SPEED_BASE	256
#define		DOWNLOAD_SPEED_MAX	10240		//最大下载速度10M

/************************inter function******************************/
void al_thread(void* param)
{
	_int32 ret = SUCCESS,sleep_count=0;
	_u32 external_ip = 0;			//sd_inet_addr("121.14.88.14")
	SOCKET sock = INVALID_SOCKET;
		
	sd_ignore_signal();
		
	ret = sd_create_socket(SD_AF_INET, SD_SOCK_RAW, SD_IPPROTO_ICMP, &sock);
	if(ret!=SUCCESS)
	{
		LOG_DEBUG("auto_lan creating socket failed:ret=%d! ",ret);
		goto FinishThread;
	}
	
	RUN_THREAD(g_al_thread_status);
	BEGIN_THREAD_LOOP(g_al_thread_status)
	if(external_ip == 0)
	{
		ret = al_start_trace(sock, &external_ip);
		
#ifdef AL_DEBUG
		if(ret == SUCCESS)
		{
			sd_assert(external_ip != 0);
			g_ping_ip=external_ip;
		}
#endif
	}

	if(ret == SUCCESS)
	{
		sd_assert(external_ip != 0);
		g_ping_delay_times = 0;
		g_max_ping_delay_time = 0;
		g_min_ping_delay_time = PING_TIMEOUT;
		ret = al_start_ping(sock, external_ip);
		if(ret == SUCCESS)
		{
			if(al_control_speed()!=SUCCESS)
			{	//可能断网了！
				LOG_ERROR("Maybe the netwaork is disconnected...");
				external_ip=0;
			}
		}
	}
	else
	{	/*trace外网ip失败，重试或者退出*/
		LOG_ERROR("auto_lan trace failed...");
	}

	/* Wait 1 second */
	sleep_count=9*g_schedule_timeout;
	while(IS_THREAD_RUN(g_al_thread_status)&&(sleep_count-->0))		
	{
		sd_sleep(100);
	}

	/* Check if the loacl ip is changed every hour */
	if(g_check_ip_count++>=3600/g_schedule_timeout)
	{
		if(al_is_ip_change())
		{
			external_ip=0;
		}
		g_check_ip_count=0;
	}
	
	ret=SUCCESS;
	END_THREAD_LOOP(g_al_thread_status)
	LOG_DEBUG("auto_lan thread had exited.");
	
FinishThread:
	STOP_THREAD(g_al_thread_status);
	finished_thread(&g_al_thread_status);
	LOG_DEBUG("auto_lan thread had finished!");
	return;
}

_int32 al_control_speed(void)
{
	_u32 upload_limit = 0, download_limit = 0;
	_int32 net_condition=al_network_condition();

	LOG_DEBUG("al_control_speed, net_condition = %d.",net_condition);
	if(AL_NET_BAD==net_condition)
	{	//网络状况不好，需要限速
		socket_proxy_get_speed_limit(&download_limit, &upload_limit);
		LOG_DEBUG("the lan is bad, download_limit = %u, upload_limit = %u.", download_limit, upload_limit);
		if(g_upload_limit_speed > UPLOAD_SPEED_MIN)
		{
			if(upload_limit > UPLOAD_SPEED_BASE /*|| download_limit > DOWNLOAD_SPEED_BASE*/)
			{
				upload_limit = UPLOAD_SPEED_BASE;
				//download_limit = DOWNLOAD_SPEED_BASE;
			}
			else
			{
				upload_limit = (100 -  g_speed_down_per) * upload_limit / 100;
				if(upload_limit < UPLOAD_SPEED_MIN)
					upload_limit = UPLOAD_SPEED_MIN;
				//download_limit = (100 - g_speed_down_per) * download_limit / 100;
				//if(download_limit < DOWNLOAD_SPEED_MIN)
				//	download_limit = DOWNLOAD_SPEED_MIN;
			}
			
			LOG_DEBUG("auto lan limit speed, set download_limit = %u, set upload_limit = %u.", download_limit, upload_limit);
			socket_proxy_set_speed_limit(download_limit, upload_limit);
		}
	}
	else if(AL_NET_GOOD==net_condition)
	{	//网络状况很好，提高速度
		socket_proxy_get_speed_limit(&download_limit,  &upload_limit);
		LOG_DEBUG("the lan is good, download_limit = %u, upload_limit = %u.", download_limit, upload_limit);

		if(upload_limit < g_upload_limit_speed)
		{
			if(upload_limit > UPLOAD_SPEED_MAX/* || download_limit > DOWNLOAD_SPEED_MAX*/)
			{
				upload_limit = UPLOAD_SPEED_MAX;
				//download_limit = DOWNLOAD_SPEED_MAX;
			}
			else
			{
				upload_limit = (g_speed_up_per + 100) * upload_limit / 100;
				if(upload_limit > UPLOAD_SPEED_MAX)
					upload_limit = UPLOAD_SPEED_MAX;
				
				if(upload_limit > g_upload_limit_speed)
					upload_limit = g_upload_limit_speed;
				//download_limit = (g_speed_up_per + 100) * download_limit / 100;
				//if(download_limit > DOWNLOAD_SPEED_MAX)
				//	download_limit = DOWNLOAD_SPEED_MAX;
			}
			LOG_DEBUG("auto lan limit speed, set download_limit = %u, set upload_limit = %u.", download_limit, upload_limit);
			socket_proxy_set_speed_limit(download_limit, upload_limit);
		}
	}	
	else if(AL_NET_DISCONNECT==net_condition)
	{
		return -1;
	}	

	return SUCCESS;
}

BOOL al_is_ip_change(void)
{
	_int32 ret_val=SUCCESS;
	_u32 local_ip=0;
	char physical_addr[10]={0};
	char physical_addr_str[16]={0};
	_int32 physical_addr_len = 10;
	BOOL ret_b = FALSE;

	ret_val = get_physical_address(physical_addr, &physical_addr_len);
	if(ret_val != SUCCESS)
	{
		LOG_DEBUG("al_is_ip_change...Error when getting physical_address!");
		//sd_assert(FALSE);
		ret_b = FALSE;
	}
	else
	{
		/* convert to string */
		ret_val = str2hex(physical_addr, physical_addr_len, physical_addr_str, 15);
		if(ret_val != SUCCESS)
		{
			LOG_DEBUG("al_is_ip_change...Error when converting physical_address!");
			sd_assert(FALSE);
			return FALSE;
		}
		else
		{
			if(sd_strlen(g_physical_addr)==0)
			{
				LOG_DEBUG("al_is_ip_change...get physical_address:%s",physical_addr_str);
				sd_memcpy(g_physical_addr, physical_addr_str, 16);
				ret_b=TRUE;
			}
			else
			{
				if(0!=sd_strcmp(g_physical_addr, physical_addr_str))
				{
					LOG_DEBUG("al_is_ip_change...the physical_address changed:%s->%s",g_physical_addr,physical_addr_str);
					sd_memcpy(g_physical_addr, physical_addr_str, 16);
					ret_b=TRUE;
				}
			}
			
			local_ip = sd_get_local_ip(); 
			if(g_local_ip==0)
			{
#ifdef _DEBUG
				sd_inet_ntoa(local_ip, physical_addr_str, 15);
				LOG_DEBUG("al_is_ip_change...get local ip:%s",physical_addr_str);
#endif
				g_local_ip=local_ip;
				ret_b=TRUE;
			}
			else
			{
				if(g_local_ip!=local_ip)
				{
#ifdef _DEBUG
					sd_inet_ntoa(local_ip, physical_addr_str, 15);
					LOG_DEBUG("al_is_ip_change...local ip changed:%s",physical_addr_str);
#endif
					g_local_ip=local_ip;
					ret_b=TRUE;
				}
			}
		}
	}
	
	LOG_DEBUG("al_is_ip_change:%d", ret_b);
	return ret_b;
}


/************************out function******************************/
_int32 al_start(void)
{
	settings_get_int_item("auto_lan.speed_up_per", (_int32*)&g_speed_up_per);
	settings_get_int_item("auto_lan.speed_down_per", (_int32*)&g_speed_down_per);
	settings_get_int_item("auto_lan.max_ping_time", (_int32*)&g_max_ping_time);
	settings_get_int_item("auto_lan.min_ping_time", (_int32*)&g_min_ping_time);
	settings_get_int_item("auto_lan.schedule_timeout", (_int32*)&g_schedule_timeout);
	settings_get_int_item("system.download_limit_speed", (_int32*)&g_download_limit_speed);
	settings_get_int_item("system.upload_limit_speed", (_int32*)&g_upload_limit_speed);
	
	if (g_schedule_timeout < 1) 
	{
	    g_schedule_timeout=1;
	}

	sd_memset(g_physical_addr,0,16);
	g_local_ip = 0;
	g_check_ip_count = 0;
	
	al_is_ip_change();

	_int32 ret = sd_create_task(al_thread, 4096, NULL, &g_al_thread_id);
	CHECK_VALUE(ret);

	/* wait for thread running */
	while (g_al_thread_status == INIT)
	{
	    sd_sleep(1);
	}

	LOG_DEBUG("al_start:id=%u,status=%d...",g_al_thread_id,g_al_thread_status);
	
	if (IS_THREAD_RUN(g_al_thread_status))
	{
		g_is_auto_lan_enable=TRUE;
#ifdef AL_DEBUG
		sd_create_task(al_test_thread, 4096, NULL, &g_al_test_thread_id);
#endif
	}
	else
	{
		g_is_auto_lan_enable=FALSE;
		ret = sd_finish_task(g_al_thread_id);
		CHECK_VALUE(ret);
		g_al_thread_id=0;
		g_al_thread_status=INIT;
	}
	return SUCCESS;
}

_int32 al_stop(void)
{
	_int32 ret = SUCCESS;
	LOG_DEBUG("start stop auto_lan thread:id=%u,status=%d...",g_al_thread_id,g_al_thread_status);
	if(IS_THREAD_RUN(g_al_thread_status))
	{
		STOP_THREAD(g_al_thread_status);					/*stop all thread*/
		wait_thread(&g_al_thread_status, 0);
	}

	if(g_al_thread_id!=0)
	{
		ret = sd_finish_task(g_al_thread_id);
		CHECK_VALUE(ret);
	}

	g_al_thread_id=0;
	g_al_thread_status=INIT;
	g_is_auto_lan_enable=FALSE;
	
#ifdef AL_DEBUG
	if(IS_THREAD_RUN(g_al_test_thread_status))
	{
		STOP_THREAD(g_al_test_thread_status);					/*stop all thread*/
		wait_thread(&g_al_test_thread_status, 0);
	}

	if(g_al_test_thread_id!=0)
	{
		ret = sd_finish_task(g_al_test_thread_id);
		CHECK_VALUE(ret);
	}

	g_al_test_thread_id=0;
	g_al_test_thread_status=INIT;
#endif

	LOG_DEBUG("finish stop auto_lan thread...");
	return ret;
}

BOOL al_is_enable(void)
{
	LOG_DEBUG("al_is_enable=%d...",g_is_auto_lan_enable);
	return g_is_auto_lan_enable;
}
void al_set_max_speed_limit(_u32 download_limit_speed, _u32 upload_limit_speed)
{
	g_download_limit_speed = download_limit_speed;
	g_upload_limit_speed = upload_limit_speed;
}



_int32 al_on_ping_result(BOOL result, _u32 delay)
{
	if(result == FALSE)
		return SUCCESS;
	
	g_ping_delay_times += delay;
	
	if(g_max_ping_delay_time<delay)
	{
		g_max_ping_delay_time=delay;
	}

	if(g_min_ping_delay_time>delay)
	{
		g_min_ping_delay_time=delay;
	}
	
	LOG_DEBUG("al_on_ping_result, delay = %u, g_ping_delay_times = %u[%u,%u]", delay, g_ping_delay_times,g_min_ping_delay_time,g_max_ping_delay_time);
	return SUCCESS;
}

_int32 al_network_condition(void)
{
	_int32 net_condition=AL_NET_NORMAL;
	
	g_average_delay_time= (g_ping_delay_times-g_max_ping_delay_time-g_min_ping_delay_time) / (PING_NUM-2);
	if(g_average_delay_time >= g_max_ping_time)
	{	//网络状况不好，需要限速
		if(g_net_warning_flag!=AL_NET_BAD)
		{
			g_net_warning_flag=AL_NET_BAD;
		}
		else
		{
			g_net_warning_flag=AL_NET_NORMAL;
			if(g_average_delay_time==PING_TIMEOUT)
			{
				net_condition=AL_NET_DISCONNECT;
			}
			else
			{
			net_condition=AL_NET_BAD;
		}
	}
	}
	else if(g_average_delay_time < g_min_ping_time)
	{	//网络状况很好，提高速度
		if(g_net_warning_flag!=AL_NET_GOOD)
		{
			g_net_warning_flag=AL_NET_GOOD;
		}
		else
		{
			g_net_warning_flag=AL_NET_NORMAL;
			net_condition=AL_NET_GOOD;
		}
	}
	else
	{
		g_net_warning_flag=AL_NET_NORMAL;
	}

	LOG_DEBUG("al_network_condition, the average delay time = %u.g_net_warning_flag=%d,net_condition=%d",g_average_delay_time,g_net_warning_flag,net_condition);
	return net_condition;
}







/**************************************************************************************************************/

#ifdef AL_DEBUG

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "platform/sd_fs.h"
#include "utility/utility.h"

char * get_cur_time(void)
{

	struct tm local_time;
	struct timeval now;
	static char tme_str[100];

	gettimeofday(&now, NULL);
	localtime_r(&now.tv_sec, &local_time);
	
//{tv_sec = 1248881670, tv_usec = 678166}
// {tm_sec = 30, tm_min = 34, tm_hour = 23, tm_mday = 29, tm_mon = 6, tm_year = 109, tm_wday = 3, tm_yday = 209, tm_isdst = 0, 
 // tm_gmtoff = 28800, tm_zone = 0x863f158 "CST"}

	sd_memset(tme_str,0,100);
	sprintf(tme_str, "%04d-%02d-%02d %02d:%02d:%02d:%03ld", local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday,local_time.tm_hour, local_time.tm_min, local_time.tm_sec, now.tv_usec / 1000);

	return tme_str; 

}
void al_test_thread(void* param)
{
	_u32 download_speed=0,upload_speed=0,download_limit_speed=0,upload_limit_speed=0,ping_delay_time=0,file_id=0,writesize;
	char str_buffer[255];
	_int32 ret_val=SUCCESS;
	
	sd_ignore_signal();
		
	ret_val = sd_open_ex("al_test.txt", O_FS_CREATE, &file_id);
	if(ret_val!=SUCCESS||file_id == -1)
	{
		/* Opening File error */
		LOG_DEBUG("al_test_thread Opening File error !");
		goto FinishThread;
	}
	
	RUN_THREAD(g_al_test_thread_status);
	BEGIN_THREAD_LOOP(g_al_test_thread_status)

		if(g_ping_ip!=0)
		{
			sd_memset(str_buffer,0,255);
			sd_strncpy(str_buffer,"The external ip is:     ",22);
			sd_inet_ntoa(g_ping_ip, str_buffer+21, 200);
			sd_strcat(str_buffer,"\n",sd_strlen("\n"));
			sd_write( file_id,str_buffer, sd_strlen(str_buffer), &writesize);
			g_ping_ip=0;
		}
		download_speed = socket_proxy_speed_limit_get_download_speed();
		upload_speed = socket_proxy_speed_limit_get_upload_speed();
		socket_proxy_get_speed_limit(&download_limit_speed, &upload_limit_speed);
		ping_delay_time=(_u32)g_average_delay_time;
		
		sd_memset(str_buffer,0,255);
		sprintf(str_buffer,"%s  %8u,%8u,%8u,%8u,%8u\n", get_cur_time(),download_speed,upload_speed,download_limit_speed,upload_limit_speed,ping_delay_time); 

		sd_write( file_id,str_buffer, sd_strlen(str_buffer), &writesize);
		
		sd_sleep(1000);
	
	END_THREAD_LOOP(g_al_test_thread_status)
	LOG_DEBUG("al_test_thread thread had exited.");
	sd_close_ex(file_id);
	
FinishThread:
	STOP_THREAD(g_al_test_thread_status);
	finished_thread(&g_al_test_thread_status);
	LOG_DEBUG("al_test_thread thread had finished!");
	return;
}

#endif

#endif

