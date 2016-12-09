#include "utility/errcode.h"
#include "utility/logid.h"
#include "utility/utility.h"
#include "utility/settings.h"
#include "platform/sd_fs.h"
#include "utility/string.h"
#include "platform/sd_task.h"

#ifdef _LOGGER

static _int32 g_default_logger_level = LOG_DEBUG_LV;
static _int32 g_logid_lv[LOGID_COUNT];

#if defined(LINUX)
static FILE* g_log_file = NULL;
FILE* log_file()
{
	return g_log_file;
}
#elif defined(WINCE)
static FILE* g_log_file = NULL;
FILE* log_file()
{
	return g_log_file;
}
#endif
static _u32 g_max_filesize = 256 * 1024 * 1024;
static char g_base_log_filename[256];
static char g_switch_buffer_1[256];
static char g_switch_buffer_2[256];

#if defined(LINUX)
_u32 log_max_filesize()
{
	return g_max_filesize;
}
#endif

static TASK_LOCK g_log_lock;
TASK_LOCK *log_lock(void)
{
	return &g_log_lock;
}

_int32 current_loglv(_int32 logid)
{
	return g_logid_lv[logid];
}

const static char *g_log_desc[] = {
	"common",
	"platform",
	"asyn_frame",
	"ftp_data_pipe",
	"http_data_pipe",
	"task_manager",
	"dispatcher",
	"asyn_frame_test",
	"connect_manager",
	"file_manager",
	"socket_proxy",
	"res_query",
	"p2p_pipe",
	"data_manager",
	"correct_manager",
	"data_check",
	"data_receive",
	"file_info",
	"range_manager",
	"data_buffer",
	"settings",
	"range_list",
	"socket_reactor",
	"fs_reactor",
	"dns_reactor",
	"device_handler",
	"mempool",
	"msg_list",
	"timer",
	"download_task",
	"bt_download",
	"reporter",
	"p2p_transfer_layer",
	"upload_manager",
	"p2sp_task",
	"vod_data_manager",
	"index_parser",
	"http_server",
	"auto_lan",
	"dns_parser",
	"emule",
	"dk_query",
	"cmd_interface",
	"em_common",
	"em_interface",
	"em_asyn_frame",
	"em_scheduler",
	"em_settings",
	"em_mempool",
	"em_msg_list",
	"em_timer",
	"em_download_task",
	"em_tree_manager",
	"em_vod_manager",
	"em_remote_control",
	"bitmap",
	"canvas",
	"eui",
	"event",
	"event_driver",
	"event_processer",
	"event_queue",
	"gal",
	"graphic",
	"ial",
	"widget",
	"window",
	"cursor",
	"screen",
	"shortcut",
	"cache",
	"symbian_appui",
	"symbian_view",
	"ui",
	"play",
	"ui_paint_func",
	"text_render",
	"member",
	"xml",
	"minibrowser",
	"import_file",
	"xmp",
	"drm",
	"vulu",
	"kankan",
	"ekk_player",
	"ui_interface", 
	"json_interface",
	"high_speed_channel",
	"lixian",
	"bt_pipe",
	"bt_magnet",
	"bencode",
	"torrent_parser",
	"bt_file_manager",
	"opengl",
	"walk_box",
	"neighbor",
	"unknown",
	"unknown",
	"em_reactor",
	"box_login",
	"env_detector",
	"local_control",
	"json_lib",
	"netstatcheck",
	"em_reporter"
};

const char* get_logdesc(_int32 id)
{
	if(id >= sizeof(g_log_desc) || id < 0)
		return "UNKNOWN MODULE";
	else
		return g_log_desc[id];
}
const char * get_base_log_filename(void)
{
	return g_base_log_filename;
}
static void handle_sub_lv(_int32 lv, char *sub_lv_conf, _int32 bufsize)
{
	_int32 idx = 0, int_idx = 0, logid = 0;
	char int_buf[10];
	for(idx = 0; idx < bufsize && sub_lv_conf[idx]; idx++)
	{
		if(sub_lv_conf[idx] <= '9' && sub_lv_conf[idx] >= '0')
		{
			if(int_idx >= 10)
			{
				sd_assert(FALSE);
				break;
			}

			int_buf[int_idx++] = sub_lv_conf[idx];
		}
		else
		{
			if(int_idx > 0)
			{
				int_buf[int_idx] = 0;
				logid = sd_atoi(int_buf);
				if(logid >= 0 && logid < LOGID_COUNT)
				{
					g_logid_lv[logid] = lv;
				}
				int_idx = 0;
			}
		}
	}

	if(int_idx > 0)
	{
		int_buf[int_idx] = 0;
		logid = sd_atoi(int_buf);
		if(logid >= 0 && logid < LOGID_COUNT)
		{
			g_logid_lv[logid] = lv;
		}
	}
}


void switch_file(_int32 cur_file_idx)
{
	sd_snprintf(g_switch_buffer_2, sizeof(g_switch_buffer_2), "%s.%d", g_base_log_filename, cur_file_idx + 1);

	if(sd_file_exist(g_switch_buffer_2))
	{
		switch_file(cur_file_idx + 1);
	}

	sd_snprintf(g_switch_buffer_1, sizeof(g_switch_buffer_1), "%s.%d", g_base_log_filename, cur_file_idx);
	sd_snprintf(g_switch_buffer_2, sizeof(g_switch_buffer_2), "%s.%d", g_base_log_filename, cur_file_idx + 1);

	sd_rename_file(g_switch_buffer_1, g_switch_buffer_2);
}

#if defined(LINUX)
void check_logfile_size(void)
#elif defined(WINCE)
void check_logfile_size(void)
#endif
{
#if defined(LINUX) || defined(WINCE)
	if(g_log_file != stdout)
	{
		if(ftell(g_log_file) >= g_max_filesize)
		{
			sd_snprintf(g_switch_buffer_2, sizeof(g_switch_buffer_2), "%s.%d", g_base_log_filename, 1);
			if(sd_file_exist(g_switch_buffer_2))
			{
				switch_file(1);
			}
			sd_snprintf(g_switch_buffer_2, sizeof(g_switch_buffer_2), "%s.%d", g_base_log_filename, 1);

			fclose(g_log_file);
			sd_rename_file(g_base_log_filename, g_switch_buffer_2);

			g_log_file = fopen(g_base_log_filename, "a+");
			if(g_log_file)
			{
				/* set line-buffer, not care of the return-value */
#if defined(LINUX)
				setvbuf(g_log_file, NULL, _IOLBF, 0);
#endif
				//setbuf(g_log_file, NULL);
			}
			else
				g_log_file = stdout;
		}
	}

#endif
}

static void extract_int(const char *buffer, _int32 *data)
{
	_int32 idx = 0, int_idx = 0;
	char tmp_buf[11];
	for(idx = 0; buffer[idx]; idx ++)
	{
		if(buffer[idx] >= '0' && buffer[idx] <= '9')
		{
			tmp_buf[int_idx++] = buffer[idx];
			if(int_idx > 9)
				break;
		}
		else if(int_idx > 0 )
			break;
	}

	tmp_buf[int_idx] = 0;
	if(int_idx > 0)
		*data = sd_atoi(tmp_buf);
}

#define WHITE_SPACE(ch)	((ch) == 0x20 || (ch) == '\t')
#define END_OF_LINE(ch)	(!(ch) || (ch) == '\r' || (ch) == '\n')

static void extract_string(const char *src_buf, char *buffer, _int32 bufsize)
{
	_int32 idx = 0, des_idx = 0;
	/* erase prefix white-space */
	for(idx = 0; WHITE_SPACE(src_buf[idx]); idx++)
	{
	}

	for(des_idx = 0; des_idx < bufsize && !END_OF_LINE(src_buf[idx]); idx++, des_idx++)
		buffer[des_idx] = src_buf[idx];

	/* erase suffix white-space */
	for(des_idx--; des_idx >= 0 && WHITE_SPACE(buffer[des_idx]); des_idx--)
	{
	}

	buffer[des_idx + 1] = 0;
}

static const char* find_value(const char *buffer)
{
	_int32 idx = 0;
	for(idx = 0; !END_OF_LINE(buffer[idx]); idx++)
	{
		if(buffer[idx] == '=')
			return buffer + idx + 1;
	}

	return buffer + idx;
}

#endif

_int32 sd_log_init(const char *conf_file)
{

#ifdef _LOGGER
#if defined(LINUX) || defined(WINCE)
	_int32 idx = 0, ret_val = SUCCESS;
	BOOL default_loginfo = TRUE;
	FILE *fp = NULL;
	char buffer[256], line[256];
	const char *pvalue=NULL;
#if defined(_ANDROID_LINUX)
	const char * default_log_setting ="logger.default.level=2\r\nlogger.debug.level=\r\nlogger.error.level=\r\nlogger.off.level=\r\nlogger.logfile=/sdcard/Thunder.txt\r\n";
#elif defined(MACOS)&&defined(MOBILE_PHONE)
	char default_log_setting[MAX_FULL_PATH_BUFFER_LEN*2]= "logger.default.level=0\r\nlogger.debug.level=\r\nlogger.error.level=\r\nlogger.off.level=\r\nlogger.logfile=Thunder.txt\r\n";
	char * p_pos = NULL;
	//sd_strcat(default_log_setting,conf_file,sd_strlen(conf_file));
	//p_pos = sd_strrchr(default_log_setting, '/');
	//sd_assert(p_pos!=NULL);
	//p_pos[1] = '\0';
	//sd_strcat(default_log_setting,"Thunder.txt\r\n", sd_strlen("Thunder.txt\r\n"));
#else
	const char * default_log_setting ="logger.default.level=1\r\nlogger.debug.level=\r\nlogger.error.level=\r\nlogger.off.level=\r\nlogger.logfile=Thunder.txt\r\n";
#endif
	if(conf_file)
	{
		fp = fopen(conf_file, "rb");
		if(fp)
		{
			ret_val = fread(buffer, 1, sizeof(buffer), fp);
			if(ret_val > 0)
			{
				if(ret_val < sizeof(buffer))
					buffer[ret_val] = 0;
				else
					buffer[ret_val - 1] = 0;

				default_loginfo = FALSE;
			}
			fclose(fp);
		}
		else
		{
			sd_memset(buffer, 0x00,256);
			sd_strncpy(buffer, default_log_setting, 256);
			default_loginfo = FALSE;
		}
	}

	if(default_loginfo)
		g_default_logger_level = LOG_DEBUG_LV;
	else
	{
		pvalue = sd_strstr(buffer, "logger.default.level", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_int(pvalue, &g_default_logger_level);
		}
	}

	for(idx = 0; idx < LOGID_COUNT; idx++)
		g_logid_lv[idx] = g_default_logger_level;

	if(!default_loginfo)
	{
		pvalue = sd_strstr(buffer, "logger.debug.level", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_string(pvalue, line, sizeof(line));
			handle_sub_lv(LOG_DEBUG_LV, line, sizeof(line));
		}

		pvalue = sd_strstr(buffer, "logger.error.level", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_string(pvalue, line, sizeof(line));
			handle_sub_lv(LOG_ERROR_LV, line, sizeof(line));
		}

		pvalue = sd_strstr(buffer, "logger.off.level", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_string(pvalue, line, sizeof(line));
			handle_sub_lv(LOG_OFF_LV, line, sizeof(line));
		}

		pvalue = sd_strstr(buffer, "logger.filesize", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_int(pvalue, (_int32*)&g_max_filesize);
			if(g_max_filesize > 0 && g_max_filesize < 1024)
				g_max_filesize *= (1024 * 1024);
			else
				g_max_filesize = 256 * 1024 * 1024;
		}

		pvalue = sd_strstr(buffer, "logger.logfile", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_string(pvalue, line, sizeof(line));

			if(line[0] != 0)
			{
#if defined(MACOS)&&defined(MOBILE_PHONE)
				sd_strncpy(g_base_log_filename, conf_file, sd_strlen(conf_file));
				//p_pos = sd_strrchr(g_base_log_filename, '/');
				p_pos = sd_strstr(g_base_log_filename,"/.con",0);
				sd_assert(p_pos!=NULL);
				p_pos[1] = '\0';
				sd_strcat(g_base_log_filename,line, sd_strlen(line));
#else
				sd_strncpy(g_base_log_filename, line, sizeof(g_base_log_filename));
				g_base_log_filename[sizeof(g_base_log_filename) - 1] = 0;
#endif			

#ifdef WINCE 
                printf("\n open log file %s", g_base_log_filename);
#endif
				g_log_file = fopen(g_base_log_filename, "a+");
				if(g_log_file)
				{
					/* set line-buffer, not care of the return-value */
#if defined(LINUX)
					setvbuf(g_log_file, NULL, _IOLBF, 0);
#endif
					//setbuf(g_log_file, NULL);
				}
			}
		}
	}

	if(!g_log_file)
		g_log_file = stdout;

	ret_val = sd_init_task_lock(&g_log_lock);
	CHECK_VALUE(ret_val);

#endif

#endif	// _LOGGER
	return SUCCESS;
}

_int32 sd_log_reload(const char * conf_file)
{
#ifdef _LOGGER

#if defined(LINUX) || defined(WINCE)
	_int32 idx = 0, ret_val = SUCCESS;
	BOOL default_loginfo = TRUE;
	FILE *fp = NULL;
	char buffer[256], line[256];
	const char *pvalue=NULL;
#if defined(_ANDROID_LINUX)
	const char * default_log_setting ="logger.default.level=2\r\nlogger.debug.level=\r\nlogger.error.level=\r\nlogger.off.level=\r\nlogger.logfile=/sdcard/Thunder.txt\r\n";
#elif defined(MACOS)&&defined(MOBILE_PHONE)
	char default_log_setting[MAX_FULL_PATH_BUFFER_LEN*2]= "logger.default.level=1\r\nlogger.debug.level=\r\nlogger.error.level=\r\nlogger.off.level=\r\nlogger.logfile=Thunder.txt\r\n";
	char * p_pos = NULL;
	//sd_strcat(default_log_setting,conf_file,sd_strlen(conf_file));
	//p_pos = sd_strrchr(default_log_setting, '/');
	//sd_assert(p_pos!=NULL);
	//p_pos[1] = '\0';
	//sd_strcat(default_log_setting,"Thunder.txt\r\n", sd_strlen("Thunder.txt\r\n"));
#else
	const char * default_log_setting ="logger.default.level=1\r\nlogger.debug.level=\r\nlogger.error.level=\r\nlogger.off.level=\r\nlogger.logfile=Thunder.txt\r\n";
#endif

	FILE* _log_file = NULL;
	FILE *tmp;
	
	if(conf_file)
	{
		fp = fopen(conf_file, "rb");
		if(fp)
		{
			ret_val = fread(buffer, 1, sizeof(buffer), fp);
			if(ret_val > 0)
			{
				if(ret_val < sizeof(buffer))
					buffer[ret_val] = 0;
				else
					buffer[ret_val - 1] = 0;

				default_loginfo = FALSE;
			}
			fclose(fp);
		}
		else
		{
			sd_memset(buffer, 0x00,256);
			sd_strncpy(buffer, default_log_setting, 256);
			default_loginfo = FALSE;
		}
	}
#if 1
	g_default_logger_level = LOG_DEBUG_LV;
#else

	if(default_loginfo)
		g_default_logger_level = LOG_DEBUG_LV;
	else
	{
		pvalue = sd_strstr(buffer, "logger.default.level", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_int(pvalue, &g_default_logger_level);
		}
	}
#endif
	for(idx = 0; idx < LOGID_COUNT; idx++)
		g_logid_lv[idx] = g_default_logger_level;

	if(!default_loginfo)
	{
		pvalue = sd_strstr(buffer, "logger.debug.level", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_string(pvalue, line, sizeof(line));
			handle_sub_lv(LOG_DEBUG_LV, line, sizeof(line));
		}

		pvalue = sd_strstr(buffer, "logger.error.level", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_string(pvalue, line, sizeof(line));
			handle_sub_lv(LOG_ERROR_LV, line, sizeof(line));
		}

		pvalue = sd_strstr(buffer, "logger.off.level", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_string(pvalue, line, sizeof(line));
			handle_sub_lv(LOG_OFF_LV, line, sizeof(line));
		}

		pvalue = sd_strstr(buffer, "logger.filesize", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_int(pvalue, (_int32*)&g_max_filesize);
			if(g_max_filesize > 0 && g_max_filesize < 1024)
				g_max_filesize *= (1024 * 1024);
			else
				g_max_filesize = 256 * 1024 * 1024;
		}

		pvalue = sd_strstr(buffer, "logger.logfile", 0);
		if(pvalue)
		{
			pvalue = find_value(pvalue);
			extract_string(pvalue, line, sizeof(line));

			if(line[0] != 0)
			{
#if defined(MACOS)&&defined(MOBILE_PHONE)
				sd_strncpy(g_base_log_filename, conf_file, sd_strlen(conf_file));
				p_pos = sd_strrchr(g_base_log_filename, '/');
				sd_assert(p_pos!=NULL);
				p_pos[1] = '\0';
				sd_strcat(g_base_log_filename,line, sd_strlen(line));
#else
				sd_strncpy(g_base_log_filename, line, sizeof(g_base_log_filename));
				g_base_log_filename[sizeof(g_base_log_filename) - 1] = 0;
#endif			

#ifdef WINCE 
                printf("\n open log file %s", g_base_log_filename);
#endif
				_log_file = fopen(g_base_log_filename, "a+");
				if(_log_file)
				{
					/* set line-buffer, not care of the return-value */
#if defined(LINUX)
					setvbuf(_log_file, NULL, _IOLBF, 0);
#endif
					//setbuf(g_log_file, NULL);
				}
			}
		}
	}

	if(!_log_file)
		_log_file = stdout;

	// exchange _log_file handle
	tmp = g_log_file;
	g_log_file = _log_file;
	if (tmp != stdout && tmp != NULL)
	{
		fclose(tmp);
	}

	return 0;
#else
	// not impl yet
	return 0;
#endif //defined(LINUX) || defined(WINCE)
#else
	return 0;
#endif // _LOGGER
}

_int32 sd_log_uninit(void)
{
	_int32 ret = SUCCESS;
	
#ifdef _LOGGER
#if defined(LINUX)|| defined(WINCE)
	if(g_log_file && g_log_file != stdout)
	{
		fclose(g_log_file);
		g_log_file = stdout;
	}
#endif
	sd_uninit_task_lock(&g_log_lock);
#endif /* _LOGGER  */



	return ret;
}
