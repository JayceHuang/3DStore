#include "utility/define.h"

#ifdef ENABLE_MEMBER

#include "member/member_impl.h"
#include "member/member_protocal_interface.h"
#include "member/member_protocal_impl.h"
#include "member/member_interface.h"
#include "member/member_utility.h"

#include "utility/utility.h"
#include "utility/string.h"
#include "utility/version.h"
#include "utility/define_const_num.h"
#include "utility/mempool.h"
#include "utility/sd_iconv.h"
#include "utility/peerid.h"
#include "utility/time.h"

#include "tree_manager/tree_interface.h"
#include "tree_manager/tree_impl.h"

#include "embed_thunder_version.h"
#include "scheduler/scheduler.h"

#include "em_common/em_define.h"
#include "em_common/em_errcode.h"
#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif
#define EM_LOGID LOGID_MEMBER
#include "em_common/em_logger.h"

	
#define	LOGINING_TIMEOUT		10000
#define	LOGOUT_TIMEOUT		10000
#define	MEMBER_PING_TIMEOUT	300000

static	MEMBER_INFO		g_member_info;
static	MEMBER_DATA		g_member_data;
static	BOOL 	g_icon_callback = FALSE;
static	BOOL 	g_using_login_cache = FALSE;	 /* 是否正在使用登录态缓存机制 */

_int32 init_member_modular_impl(void)
{
	char version[MAX_VERSION_LEN] = {0};
	char* path = NULL;
	_int32 path_len = 0;
	
	sd_memset(&g_member_info, 0, sizeof(g_member_info));
	sd_memset(&g_member_data, 0, sizeof(g_member_data));
	g_member_data._timer_id = MEMBER_INVALID_TIMER_ID;
	g_member_data._failed = 0;
	g_member_data._type = 0;
	get_version(version, MAX_VERSION_LEN);
	/* 最多31字节 */
	sd_strncpy(g_member_data._version, version, 31);
	g_member_data._state = MEMBER_INIT_STATE;
	//g_member_data._callback_func = callback_func;
	g_icon_callback = FALSE;
	g_using_login_cache = FALSE;

	path = em_get_system_path();
	path_len = sd_strlen(path);
	return member_protocal_interface_init(path, path_len);
}

_int32 uninit_member_modular_impl(void)
{
	g_icon_callback = FALSE;
	g_using_login_cache = FALSE;
	g_member_data._callback_func = NULL;
	member_protocal_interface_uninit();
	//member_stop_timer();
	return SUCCESS;
}

_int32 member_set_callback_impl(member_event_notify callback_func)
{
	g_member_data._callback_func = callback_func;
	return SUCCESS;
}

_int32 member_login_impl(_u32 type, char* username, _u32 username_len, char* pwd, _u32 pwd_len)
{
	//发送login数据包
	_int32 ret = SUCCESS;
	char peer_id[PEER_ID_SIZE+4] = {0};
	if(g_member_data._state != MEMBER_INIT_STATE && g_member_data._state != MEMBER_FAILED_STATE && g_member_data._state != MEMBER_LOGOUT_STATE)
	{
		//sd_assert(FALSE);
		EM_LOG_ERROR("member_login_impl, state=%d",  g_member_data._state);
		return MEMBER_PROTOCAL_ERROR_OPER_DENIED;
	}
	g_icon_callback = FALSE;
	g_using_login_cache = FALSE;
	sd_memset(&g_member_data._login_info,0x00,sizeof(MEMBER_PROTOCAL_LOGIN_INFO));
	g_member_data._login_info._login_type = 0;
	g_member_data._login_info._business_type = MEMBER_BUSSINESS_TYPE;
	sd_memcpy(g_member_data._login_info._user_name, username, MIN(username_len, MEMBER_PROTOCAL_USER_NAME_SIZE - 1));
	g_member_data._login_info._user_name_len = username_len;
	sd_memcpy(g_member_data._login_info._passwd, pwd, MIN(pwd_len, MEMBER_PROTOCAL_PASSWD_SIZE - 1));
	g_member_data._login_info._passwd_len = pwd_len;
	
	get_peerid(peer_id, PEER_ID_SIZE);
	sd_memcpy(g_member_data._login_info._peerid, peer_id, sd_strlen(peer_id));
	g_member_data._login_info._peerid_len = 16;
	g_member_data._state = MEMBER_LOGINING_STATE;
	
	sd_assert(sd_strlen(g_member_info._session_id)==0);
	if(sd_strlen(g_member_info._session_id)!=0)
	{
		g_member_info._session_id[0] = '\0';
		g_member_info._session_id_len = 0;
	}

	if(member_has_login_info(g_member_data._login_info._user_name))
	{
		/* 已有缓存登录态 */
		ret = member_protocal_interface_login(&g_member_data._login_info);
	}
	else
	{
		ret = member_protocal_interface_first_login(&g_member_data._login_info);
	}
	if(ret != SUCCESS)
	{
		g_member_data._state = MEMBER_INIT_STATE;
		EM_LOG_ERROR("member_login_impl, ret=%d", ret);
		return ret;
	}
//	member_start_timer(LOGINING_TIMEOUT);
	return SUCCESS;
}

_int32 member_login_sessionid_impl(char* sessionid,  _u32 sessionid_len, _u64 origin_user_id)
{
	//发送login数据包
	_int32 ret = SUCCESS;
	char peer_id[PEER_ID_SIZE+4] = {0};

	if(sessionid_len >= MEMBER_PROTOCAL_WEB_SESSION_ID_SIZE)
	{
		return INVALID_ARGUMENT;
	}
	
	if(g_member_data._state != MEMBER_INIT_STATE && g_member_data._state != MEMBER_FAILED_STATE && g_member_data._state != MEMBER_LOGOUT_STATE)
	{
		EM_LOG_ERROR("member_login_sessionid_impl, state=%d",  g_member_data._state);
		return MEMBER_PROTOCAL_ERROR_OPER_DENIED;
	}
	
	g_icon_callback = FALSE;
	// fill in info to the g_member_data._sessionid_login_info
	sd_memset(&g_member_data._sessionid_login_info, 0x00, sizeof(MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO));
	g_member_data._sessionid_login_info._sessionid_type = 0;  /* sessionid登录时，sessionid来源于页面 */
	g_member_data._sessionid_login_info._business_type = MEMBER_BUSSINESS_TYPE;

	sd_memcpy(g_member_data._sessionid_login_info._sessionid, sessionid, MIN(sessionid_len, MEMBER_PROTOCAL_WEB_SESSION_ID_SIZE - 1));
	g_member_data._sessionid_login_info._sessionid_len = sessionid_len;
	
	get_peerid(peer_id, PEER_ID_SIZE);
	sd_memcpy(g_member_data._sessionid_login_info._peerid, peer_id, sd_strlen(peer_id));
	g_member_data._sessionid_login_info._peerid_len = 16;
	
	g_member_data._sessionid_login_info._origin_user_id = origin_user_id;
	g_member_data._sessionid_login_info._origin_business_type = 0;
	
	// update the g_member_data._state is loging
	g_member_data._state = MEMBER_LOGINING_STATE;
	
	ret = member_protocal_interface_sessionid_login(&g_member_data._sessionid_login_info);

	if(ret != SUCCESS)
	{
		g_member_data._state = MEMBER_INIT_STATE;
		EM_LOG_ERROR("member_login_sessionid_impl, ret=%d", ret);
		return ret;
	}
	return SUCCESS;
}

_int32 member_relogin_impl(void)
{
	_int32 ret_val = SUCCESS;
	
	char username[MEMBER_PROTOCAL_USER_NAME_SIZE];
	_u32 username_len;
	char passwd[MEMBER_PROTOCAL_PASSWD_SIZE];
	_u32 passwd_len;	

	username_len = g_member_data._login_info._user_name_len;
	passwd_len = g_member_data._login_info._passwd_len;
	sd_memcpy(username, g_member_data._login_info._user_name, username_len);
	sd_memcpy(passwd, g_member_data._login_info._passwd, passwd_len);

	member_logout_impl();

	member_remove_login_info(username);
	
	ret_val = member_login_impl(0, username, username_len, passwd, passwd_len);
	
	return ret_val;
}


_int32 member_logout_impl(void)
{
	MEMBER_PROTOCAL_LOGOUT_INFO logout_info;
	
	EM_LOG_DEBUG("member_logout_impl, g_icon_callback=%d,state=%u,g_using_login_cache=%d", g_icon_callback,g_member_data._state,g_using_login_cache);
	g_icon_callback = FALSE;
	if(g_member_data._state != MEMBER_LOGINED_STATE)
	{
		//sd_assert(FALSE);
		g_using_login_cache = FALSE;
		g_member_data._state = MEMBER_LOGOUT_STATE;
		//把member info 清空
		sd_memset(&g_member_info, 0, sizeof(g_member_info));
		member_protocal_interface_change_login_type(0);
		return SUCCESS;
	}
	if(!g_using_login_cache)
	{
		logout_info._userno = g_member_data._ping_info._userno;
		sd_memcpy(logout_info._session_id, g_member_data._ping_info._session_id, MEMBER_PROTOCAL_SESSION_ID_SIZE);
		logout_info._session_id_len = g_member_data._ping_info._session_id_len;
		member_protocal_interface_logout(&logout_info);
	}
	else
	{
		g_using_login_cache = FALSE;
	}
	g_member_data._state = MEMBER_LOGOUT_STATE;
	//注销后就把member info 清空
	sd_memset(&g_member_info, 0, sizeof(g_member_info));
	//既然已经注销了那就取消掉ping的超时服务器
	//member_stop_timer();
	//g_member_data._callback_func(MEMBER_LOGOUT_EVENT, SUCCESS);
	return SUCCESS;
}


_int32 member_report_download_file_impl(_u64 filesize, _u8 cid[40], char* url, _u32 url_len)
{
	MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE data;
	sd_memset(&data, 0, sizeof(data));
	data._userno = g_member_info._userid;
	sd_memcpy(data._session_id, g_member_data._ping_info._session_id, MEMBER_PROTOCAL_SESSION_ID_SIZE);
	data._session_id_len = g_member_data._ping_info._session_id_len;
	data._dl_file_size = filesize;
	sd_memcpy(data._cid, cid, 40);
	data._cid_len = 40;
	sd_strncpy(data._url, url, MIN(MEMBER_PROTOCAL_URL_SIZE, url_len));
	data._url_len = MIN(MEMBER_PROTOCAL_URL_SIZE, url_len);
	data._dead_link = 0;
	data._saved_time = 0;
	return member_protocal_interface_report_download_file(&data);
}

_int32 member_register_check_id_impl(char *user_name, member_register_callback callback_func, void *user_data)
{
	MEMBER_PROTOCAL_REGISTER_CHECK_ID *check_id = NULL;
	//char md5_one_time[33] = {0}, md5_two_time[33] = {0};
	_int32 ret = SUCCESS;

	if (user_name == NULL || callback_func == NULL)
		return -1;

	ret = sd_malloc(sizeof(*check_id), (void**)&check_id);
	CHECK_VALUE(ret);
	sd_memset(check_id, 0, sizeof(*check_id));

	check_id->_callback_func = callback_func;
	check_id->_user_data = user_data;
	sd_strncpy(check_id->_userid, user_name, MIN(sd_strlen(user_name), MEMBER_PROTOCAL_USER_NAME_SIZE - 1));
	check_id->_usertype = -1;
	
	return member_protocal_interface_register_check_id(check_id);
}

_int32 member_register_impl(char *user_name, char *password, char *nickname,
	member_register_callback callback_func, void *user_data, BOOL is_utf8)
{
	MEMBER_PROTOCAL_REGISTER_INFO *register_info = NULL;
	char md5_one_time[33] = {0}, md5_two_time[33] = {0};
	_int32 ret = SUCCESS;

	if (user_name == NULL || password == NULL || nickname == NULL || callback_func == NULL)
		return -1;

	ret = sd_malloc(sizeof(*register_info),(void**) &register_info);
	CHECK_VALUE(ret);
	sd_memset(register_info, 0, sizeof(*register_info));

	register_info->_callback_func = callback_func;
	register_info->_user_data = user_data;
	sd_strncpy(register_info->_userid, user_name, MIN(sd_strlen(user_name), MEMBER_PROTOCAL_USER_NAME_SIZE - 1));
	register_info->_usertype = 0;
	
	encode_password_by_md5(password, sd_strlen(password), md5_one_time);
	encode_password_by_md5(md5_one_time, sd_strlen(md5_one_time), md5_two_time);
	sd_strncpy(register_info->_password, md5_two_time, 33);

	register_info->_registertype = 2; //注册类型0:网游注册,1:迅雷注册,2:ipad
	sd_strncpy(register_info->_nickname, nickname, MIN(MEMBER_PROTOCAL_USER_NAME_SIZE - 1, sd_strlen(nickname)));
	
	return member_protocal_interface_register(register_info, is_utf8);
}

_int32 get_member_info_impl(MEMBER_INFO* info)
{
	if(g_member_data._state != MEMBER_LOGINED_STATE)
		return MEMBER_PROTOCAL_ERROR_OPER_DENIED;
	sd_memcpy(info, &g_member_info, sizeof(g_member_info));
	return SUCCESS;
}

_int32 member_refresh_user_info_impl(member_refresh_notify func)
{
	_int32 ret = SUCCESS;

	ret = member_protocal_interface_refresh_user_info(func);
	
	return ret;
}

_int32 get_member_picture_filepath_impl(char* filepath)
{
	_u32 len = 0;
	char path[MAX_FILE_PATH_LEN] = {0};
	if(g_member_data._state != MEMBER_LOGINED_STATE)
		return MEMBER_PROTOCAL_ERROR_OPER_DENIED;
	len = sd_strlen(g_member_info._picture_filename);

	if(len > 0)
	{
		return sd_strncpy(filepath, g_member_info._picture_filename, MAX_FILE_PATH_LEN*2);
	}
	else
	{
		return -1;
	}
//	return sd_strncpy(filename, g_member_info._picture_filename, len + 1);
//	return eui_sd_snprintf(filename, filename_len, "%u.jpg", g_member_info._userid);
}

BOOL is_member_logined_impl(void)
{
	if(g_member_data._state == MEMBER_LOGINED_STATE)
		return TRUE;
	else
		return FALSE;
}

BOOL is_member_logining_impl(void)
{
	if(g_member_data._state == MEMBER_LOGINING_STATE)
		return TRUE;
	else
		return FALSE;
}

MEMBER_STATE get_member_state_impl(void)
{
	return g_member_data._state;
}

_int32 member_handle_error(_int32 errcode)
{	
	//_int32 ret = SUCCESS;
	MEMBER_STATE state = g_member_data._state;
	g_member_data._state = MEMBER_FAILED_STATE;
	g_member_data._failed = 0;
	//member_stop_timer();
	//如果已经准备退出手机迅雷了，则不用通知上层
	if(g_member_data._callback_func == NULL)
		return SUCCESS;		
	//通知上层出错了
	switch(state)
	{
		case MEMBER_LOGINING_STATE:
			g_member_data._callback_func(LOGIN_FAILED_EVENT, errcode);
			break;
		case MEMBER_LOGINED_STATE:
			if(errcode == MEMBER_PROTOCAL_ERROR_KICK_OUT)
				g_member_data._callback_func(KICK_OUT_EVENT, errcode);
			else if(errcode == MEMBER_PROTOCAL_ERROR_NEED_RETRY)
				g_member_data._callback_func(NEED_RELOGIN_EVENT, errcode);
			else
				sd_assert(FALSE);
//				g_member_data._callback_func(PING_FAILED_EVENT, errcode);
			break;
		case MEMBER_LOGOUT_STATE:
//			g_member_data._callback_func(LOGOUT_FAILED_EVENT, errcode);
			break;
		default:
			sd_assert(FALSE);
	}
	return SUCCESS;
}

_int32 member_keepalive(void)
{
	//五分钟去ping 一次
	//member_start_timer(MEMBER_PING_TIMEOUT);
	if(g_member_data._ping_info._userno == 0 || g_member_data._ping_info._session_id_len == 0 )
		return MEMBER_PROTOCAL_ERROR_UNLOGIN;
	member_protocal_interface_ping(&g_member_data._ping_info);
	//++g_member_data._failed;
	return SUCCESS;
}

_int32 member_protocal_interface_login_basic_resp( MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info)
{
	sd_assert(sd_strlen(g_member_info._session_id)==0);

	g_member_info._userid = p_member_protocal_user_info->_userno;
	sd_strncpy(g_member_info._session_id, p_member_protocal_user_info->_session_id, p_member_protocal_user_info->_session_id_len);
	g_member_info._session_id_len = p_member_protocal_user_info->_session_id_len;
	
	sd_strncpy(g_member_info._jumpkey, p_member_protocal_user_info->_jumpkey, p_member_protocal_user_info->_jumpkey_len);
	g_member_info._jumpkey_len = p_member_protocal_user_info->_jumpkey_len;

	return SUCCESS;
}

_int32 member_protocal_interface_sessionid_login_resp(_int32 errcode, MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info)
{
	_u32 nickname_len = 31, username_len = 31;
	char item_name[64] = {0};
	_int32 user_rank = 0,errcode2ui = SUCCESS;

	EM_LOG_DEBUG("member_protocal_interface_sessionid_login_resp, g_member_data._state=%u,errcode=%d", g_member_data._state,errcode);
	//状态不对的话，不需要处理该回复
	if(g_member_data._state != MEMBER_LOGINING_STATE || g_member_data._callback_func == NULL)
	{	
		EM_LOG_DEBUG("member_protocal_interface_sessionid_login_resp, state=%u", g_member_data._state);
		return SUCCESS;			
	}
	
	if(errcode != SUCCESS)
	{
		g_member_data._state = MEMBER_FAILED_STATE;
		sd_memset(&g_member_data._ping_info, 0, sizeof(MEMBER_PROTOCAL_PING));
		EM_LOG_DEBUG("member_protocal_interface_sessionid_login_resp : callback to ui, state=%u,errcode=%d", LOGIN_FAILED_EVENT, errcode);
		g_member_data._callback_func(LOGIN_FAILED_EVENT, errcode);
		g_member_data._failed = 0;
		sd_memset(&g_member_info, 0, sizeof(g_member_info));
		return SUCCESS;
	}
	else if(NULL == p_member_protocal_user_info)
	{
		EM_LOG_DEBUG("member_protocal_interface_sessionid_login_resp : callback to ui, state=%u,errcode=%d", LOGIN_LOGINED_EVENT, errcode);
		sd_memset(&g_member_data._ping_info, 0, sizeof(MEMBER_PROTOCAL_PING));
		g_member_data._callback_func(LOGIN_LOGINED_EVENT, errcode);
		g_member_data._failed = 0;
		return SUCCESS;
	}
	//填充数据
	sd_memset(&g_member_info, 0, sizeof(g_member_info));
	g_member_info._is_vip = p_member_protocal_user_info->_is_vip;
	
	g_member_info._is_year = p_member_protocal_user_info->_is_year;
	
	g_member_info._is_platinum= p_member_protocal_user_info->_is_platinum;

	if(p_member_protocal_user_info->_user_type == 1)
	{
		g_member_info._is_new = FALSE;
		sd_strncpy(g_member_info._username, p_member_protocal_user_info->_user_name, p_member_protocal_user_info->_user_name_len);
	}
	else
	{
		g_member_info._is_new = TRUE;
		sd_u64toa(p_member_protocal_user_info->_user_new_no, g_member_info._username, 32, 10);
	}
	g_member_info._userid = p_member_protocal_user_info->_userno;

	if(sd_strlen(p_member_protocal_user_info->_nickname)!=0)
	{
		sd_strncpy(g_member_info._nickname, p_member_protocal_user_info->_nickname, MEMBER_MAX_USERNAME_LEN-1);
	}
	else
	{
		/*  将上次登录得到的昵称从配置文件中读出*/
		sd_snprintf(item_name, 63, "%llu.nickname",p_member_protocal_user_info->_userno);
		em_settings_get_str_item(item_name, g_member_info._nickname);
		
		if(sd_strlen(g_member_info._nickname)==0)
		{
			sd_strncpy(g_member_info._nickname,g_member_data._login_info._user_name,MEMBER_MAX_USERNAME_LEN-1);
		}
	}

	if(p_member_protocal_user_info->_rank==0)
	{
		sd_snprintf(item_name, 63, "%llu.rank",p_member_protocal_user_info->_userno);
		em_settings_get_int_item(item_name, &user_rank);
		p_member_protocal_user_info->_rank = (_u8)user_rank;
	}
	else
	{
		g_member_info._level = p_member_protocal_user_info->_rank;
	}
	g_member_info._vip_rank = p_member_protocal_user_info->_level;
	
	sd_strncpy(g_member_info._expire_date, p_member_protocal_user_info->_expire_date, p_member_protocal_user_info->_expire_date_len);
	
	get_member_military_title(g_member_info._level, g_member_info._military_title);
	g_member_info._current_account = p_member_protocal_user_info->_account;
	g_member_info._total_account = get_member_account(g_member_info._level + 1);
	g_member_info._order = p_member_protocal_user_info->_order;
	g_member_info._update_days = get_update_days(g_member_info._current_account, g_member_info._vip_rank);
	//ping数据
	g_member_data._ping_info._userno = p_member_protocal_user_info->_userno;
	sd_strncpy(g_member_data._ping_info._session_id, p_member_protocal_user_info->_session_id, MEMBER_PROTOCAL_SESSION_ID_SIZE);
	g_member_data._ping_info._session_id_len = p_member_protocal_user_info->_session_id_len;
	//通知上层	
	g_member_data._state = MEMBER_LOGINED_STATE;
	
	sd_strncpy(g_member_info._session_id, p_member_protocal_user_info->_session_id, p_member_protocal_user_info->_session_id_len);
	g_member_info._session_id_len = p_member_protocal_user_info->_session_id_len;
	
	sd_strncpy(g_member_info._jumpkey, p_member_protocal_user_info->_jumpkey, p_member_protocal_user_info->_jumpkey_len);
	g_member_info._jumpkey_len = p_member_protocal_user_info->_jumpkey_len;
	
	// save the icon into g_member_info
	if (sd_strlen( p_member_protocal_user_info->_icon._file_name)!=0)
	{
		_int32 ret = SUCCESS;
		sd_memset((void *)g_member_info._picture_filename, 0, 256);
		ret = sd_strncpy(g_member_info._picture_filename, p_member_protocal_user_info->_icon._file_path, p_member_protocal_user_info->_icon._file_path_len);
		CHECK_VALUE(ret);
		ret = sd_strcat(g_member_info._picture_filename, p_member_protocal_user_info->_icon._file_name, p_member_protocal_user_info->_icon._file_name_len);
		CHECK_VALUE(ret);

		/*  确保将头像名字存在配置文件中,以便下次获取失败时可以直接拿来用*/
		sd_snprintf(item_name, 63, "%llu.icon",g_member_info._userid);
		em_settings_set_str_item(item_name, p_member_protocal_user_info->_icon._file_name);
	}
	else
	{
		char iser_icon_name[MEMBER_PROTOCAL_FILE_NAME_SIZE] = {0};
		/*  将上次登录得到的头像从配置文件中读出*/
		sd_snprintf(item_name, 63, "%llu.icon",p_member_protocal_user_info->_userno);
		em_settings_get_str_item(item_name, iser_icon_name);
	
		if(sd_strlen(iser_icon_name)!=0&&sd_strlen(p_member_protocal_user_info->_icon._file_path)!=0)
		{
			_int32 ret = SUCCESS;
			sd_memset((void *)g_member_info._picture_filename, 0, 1024);
			ret = sd_strncpy(g_member_info._picture_filename, p_member_protocal_user_info->_icon._file_path, p_member_protocal_user_info->_icon._file_path_len);
			CHECK_VALUE(ret);
			ret = sd_strcat(g_member_info._picture_filename, iser_icon_name, sd_strlen(iser_icon_name));
			CHECK_VALUE(ret);
		}
	}
	EM_LOG_DEBUG("member_protocal_interface_sessionid_login_resp : callback to ui, state=%u,errcode=%d", LOGIN_LOGINED_EVENT, errcode);
	g_member_data._callback_func(LOGIN_LOGINED_EVENT, SUCCESS);
	g_member_data._failed = 0;	

	if((!g_icon_callback)&&(sd_strlen( p_member_protocal_user_info->_icon._file_name)!=0))
	{
		g_icon_callback = TRUE;
		EM_LOG_DEBUG("member_protocal_interface_sessionid_login_resp : callback to ui, state=%u,errcode=%d", UPDATE_PICTURE_EVENT, errcode);
		g_member_data._callback_func(UPDATE_PICTURE_EVENT, SUCCESS);
	}

#if 0
		member_keepalive();
#endif
	return SUCCESS;
}


_int32 member_protocal_interface_login_resp(_int32 errcode, MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info)
{
	_u32 nickname_len = 31, username_len = 31;
	char item_name[64] = {0};
	_int32 user_rank = 0,errcode2ui = SUCCESS;

	EM_LOG_DEBUG("member_protocal_interface_login_resp, state=%u,errcode=%d", g_member_data._state,errcode);
	//状态不对的话，不需要处理该回复
	if(g_member_data._state != MEMBER_LOGINING_STATE || g_member_data._callback_func == NULL)
		return SUCCESS;			
	//去掉定时器
//	member_stop_timer();
	if(errcode != SUCCESS)
	{
		/* 增加登录态本地存储机制 by zyq @ 20130107 */
		if((errcode!= MEMBER_PROTOCAL_ERROR_LOGIN_ACCOUNT_NOT_EXIST)&&(errcode!= MEMBER_PROTOCAL_ERROR_LOGIN_PASSWORD_WRONG)&&(errcode!= MEMBER_PROTOCAL_ERROR_LOGIN_ACCOUNT_LOCK))
		{
			_int32 ret_val = SUCCESS;
			BOOL is_session_id_updated = FALSE;
			char session_id[MEMBER_SESSION_ID_SIZE] = {0};
			char jumpkey[MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE] = {0};
			
			errcode2ui = MEMBER_PROTOCAL_ERROR_LOGIN_USE_CACHE;
			if(sd_strlen(g_member_info._session_id)!=0)
			{
				/* 虽然最终登录失败，但是sessionid已经被更新了 */
				sd_strncpy(session_id,g_member_info._session_id, MEMBER_SESSION_ID_SIZE);
				sd_strncpy(jumpkey,g_member_info._jumpkey,MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE);
				is_session_id_updated = TRUE;
				errcode2ui = MEMBER_PROTOCAL_ERROR_LOGIN_SESSIONID_UPDATED;
			}
			
			ret_val = member_load_login_info(g_member_data._login_info._user_name,g_member_data._login_info._passwd,&g_member_info);
			if(ret_val==SUCCESS)
			{
				EM_LOG_ERROR("member_protocal_interface_login_resp, use the cache!errcode=%d,ret_val=%d,user_name=%s,userid=%llu,is_vip=%d,expire_date=%s", errcode,ret_val,g_member_data._login_info._user_name,g_member_info._userid,g_member_info._is_vip,g_member_info._expire_date);
				if(is_session_id_updated)
				{
					/* 用最新的sessionid*/
					sd_strncpy(g_member_info._session_id, session_id,MEMBER_SESSION_ID_SIZE);
					g_member_info._session_id_len = sd_strlen(g_member_info._session_id);
					
					sd_strncpy(g_member_info._jumpkey,jumpkey,MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE);
					g_member_info._jumpkey_len = sd_strlen(g_member_info._jumpkey);

					/* 增加登录态本地存储机制 by zyq @ 20130107 */
					member_save_login_info(g_member_data._login_info._user_name,g_member_data._login_info._passwd,&g_member_info);
					/* End  by zyq @ 20130107 */
				}
				//ping数据
				g_member_data._ping_info._userno = g_member_info._userid;
				sd_strncpy(g_member_data._ping_info._session_id, g_member_info._session_id, MEMBER_PROTOCAL_SESSION_ID_SIZE);
				g_member_data._ping_info._session_id_len = g_member_info._session_id_len;
				
				//通知上层	
				g_member_data._state = MEMBER_LOGINED_STATE;
				g_member_data._callback_func(LOGIN_LOGINED_EVENT, errcode2ui);
				g_member_data._failed = 0;	

				if((!g_icon_callback)&&(sd_strlen(g_member_info._picture_filename)!=0))
				{
					g_icon_callback = TRUE;
					g_member_data._callback_func(UPDATE_PICTURE_EVENT, SUCCESS);
				}
				
				g_using_login_cache = TRUE;
				member_keepalive();
				
				return SUCCESS;
			}
		}
		/* End  by zyq @ 20130107 */
		member_remove_login_info(g_member_data._login_info._user_name);
		g_member_data._state = MEMBER_FAILED_STATE;
		sd_memset(&g_member_data._ping_info, 0, sizeof(MEMBER_PROTOCAL_PING));
		g_member_data._callback_func(LOGIN_FAILED_EVENT, errcode);
		g_member_data._failed = 0;
		sd_memset(&g_member_info, 0, sizeof(g_member_info));
		return SUCCESS;
	}
	//填充数据
	sd_memset(&g_member_info, 0, sizeof(g_member_info));
	g_member_info._is_vip = p_member_protocal_user_info->_is_vip;
	
	g_member_info._is_year = p_member_protocal_user_info->_is_year;
	
	g_member_info._is_platinum= p_member_protocal_user_info->_is_platinum;

	if(p_member_protocal_user_info->_user_type == 1)
	{
		g_member_info._is_new = FALSE;
		sd_strncpy(g_member_info._username, p_member_protocal_user_info->_user_name, p_member_protocal_user_info->_user_name_len);
	}
	else
	{
		g_member_info._is_new = TRUE;
		sd_u64toa(p_member_protocal_user_info->_user_new_no, g_member_info._username, 32, 10);
	}
	g_member_info._userid = p_member_protocal_user_info->_userno;

	if(sd_strlen(p_member_protocal_user_info->_nickname)!=0)
	{
		sd_strncpy(g_member_info._nickname, p_member_protocal_user_info->_nickname, MEMBER_MAX_USERNAME_LEN-1);
	}
	else
	{
		/*  将上次登录得到的昵称从配置文件中读出*/
		sd_snprintf(item_name, 63, "%llu.nickname",p_member_protocal_user_info->_userno);
		em_settings_get_str_item(item_name, g_member_info._nickname);
		
		if(sd_strlen(g_member_info._nickname)==0)
		{
			sd_strncpy(g_member_info._nickname,g_member_data._login_info._user_name,MEMBER_MAX_USERNAME_LEN-1);
		}
	}

	if(p_member_protocal_user_info->_rank==0)
	{
		sd_snprintf(item_name, 63, "%llu.rank",p_member_protocal_user_info->_userno);
		em_settings_get_int_item(item_name, &user_rank);
		p_member_protocal_user_info->_rank = (_u8)user_rank;
	}
	else
	{
	g_member_info._level = p_member_protocal_user_info->_rank;
	}
	g_member_info._vip_rank = p_member_protocal_user_info->_level;
	
	sd_strncpy(g_member_info._expire_date, p_member_protocal_user_info->_expire_date, p_member_protocal_user_info->_expire_date_len);
	
	get_member_military_title(g_member_info._level, g_member_info._military_title);
	g_member_info._current_account = p_member_protocal_user_info->_account;
	g_member_info._total_account = get_member_account(g_member_info._level + 1);
	g_member_info._order = p_member_protocal_user_info->_order;
	g_member_info._update_days = get_update_days(g_member_info._current_account, g_member_info._vip_rank);
	//ping数据
	g_member_data._ping_info._userno = p_member_protocal_user_info->_userno;
	sd_strncpy(g_member_data._ping_info._session_id, p_member_protocal_user_info->_session_id, MEMBER_PROTOCAL_SESSION_ID_SIZE);
	g_member_data._ping_info._session_id_len = p_member_protocal_user_info->_session_id_len;
	//通知上层	
	g_member_data._state = MEMBER_LOGINED_STATE;
	
	sd_strncpy(g_member_info._session_id, p_member_protocal_user_info->_session_id, p_member_protocal_user_info->_session_id_len);
	g_member_info._session_id_len = p_member_protocal_user_info->_session_id_len;
	
	sd_strncpy(g_member_info._jumpkey, p_member_protocal_user_info->_jumpkey, p_member_protocal_user_info->_jumpkey_len);
	g_member_info._jumpkey_len = p_member_protocal_user_info->_jumpkey_len;

	// add by hx 2013-9-11
	g_member_info._payid =  p_member_protocal_user_info->_payid;
	sd_strncpy(g_member_info._payname, p_member_protocal_user_info->_payid_name, p_member_protocal_user_info->_payid_name_len);
	// add by hx 2013-10-15
	if(p_member_protocal_user_info->_vas & 0x10000000 )
		g_member_info._is_son_account = TRUE;
	else
		g_member_info._is_son_account = FALSE;

	g_member_info._vas_type = p_member_protocal_user_info->_vas_type;
	// save the icon into g_member_info
	if (sd_strlen( p_member_protocal_user_info->_icon._file_name)!=0)
	{
		_int32 ret = SUCCESS;
		sd_memset((void *)g_member_info._picture_filename, 0, 256);
		ret = sd_strncpy(g_member_info._picture_filename, p_member_protocal_user_info->_icon._file_path, p_member_protocal_user_info->_icon._file_path_len);
		CHECK_VALUE(ret);
		ret = sd_strcat(g_member_info._picture_filename, p_member_protocal_user_info->_icon._file_name, p_member_protocal_user_info->_icon._file_name_len);
		CHECK_VALUE(ret);

		/*  确保将头像名字存在配置文件中,以便下次获取失败时可以直接拿来用*/
		sd_snprintf(item_name, 63, "%llu.icon",g_member_info._userid);
		em_settings_set_str_item(item_name, p_member_protocal_user_info->_icon._file_name);
	}
	else
	{
		char iser_icon_name[MEMBER_PROTOCAL_FILE_NAME_SIZE] = {0};
		/*  将上次登录得到的头像从配置文件中读出*/
		sd_snprintf(item_name, 63, "%llu.icon",p_member_protocal_user_info->_userno);
		em_settings_get_str_item(item_name, iser_icon_name);
	
		if(sd_strlen(iser_icon_name)!=0&&sd_strlen(p_member_protocal_user_info->_icon._file_path)!=0)
		{
			_int32 ret = SUCCESS;
			sd_memset((void *)g_member_info._picture_filename, 0, 1024);
			ret = sd_strncpy(g_member_info._picture_filename, p_member_protocal_user_info->_icon._file_path, p_member_protocal_user_info->_icon._file_path_len);
			CHECK_VALUE(ret);
			ret = sd_strcat(g_member_info._picture_filename, iser_icon_name, sd_strlen(iser_icon_name));
			CHECK_VALUE(ret);
		}
	}
	g_member_data._callback_func(LOGIN_LOGINED_EVENT, SUCCESS);
	g_member_data._failed = 0;	

	if((!g_icon_callback)&&(sd_strlen( p_member_protocal_user_info->_icon._file_name)!=0))
	{
		g_icon_callback = TRUE;
		g_member_data._callback_func(UPDATE_PICTURE_EVENT, SUCCESS);
	}

	/* 增加登录态本地存储机制 by zyq @ 20130107 */
	member_save_login_info(g_member_data._login_info._user_name,g_member_data._login_info._passwd,&g_member_info);
	/* End  by zyq @ 20130107 */

	// 增加高速通道
#ifdef ENABLE_HSC
	et_hsc_set_user_info(g_member_info._userid,g_member_info._jumpkey,g_member_info._jumpkey_len);
#endif

//#if 0
		member_keepalive();
//#endif
	return SUCCESS;
}

_int32 member_protocal_interface_user_info_resp(_int32 errcode, MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info)
{
	_u32 nickname_len = 31, username_len = 31;
	char item_name[64] = {0};
	EM_LOG_DEBUG("member_protocal_interface_user_info_resp, state=%u,errcode=%d", g_member_data._state,errcode);
	//状态不对的话，不需要处理该回复
	if((g_member_data._state != MEMBER_LOGINING_STATE && g_member_data._state != MEMBER_LOGINED_STATE) || errcode != SUCCESS)
		return SUCCESS;			

	//sd_utf8_2_gbk(p_member_protocal_user_info->_nickname, p_member_protocal_user_info->_nickname_len, g_member_info._nickname, &nickname_len);
	sd_strncpy(g_member_info._nickname, p_member_protocal_user_info->_nickname, MEMBER_MAX_USERNAME_LEN-1);
	g_member_info._level = p_member_protocal_user_info->_rank;
	
	get_member_military_title(g_member_info._level, g_member_info._military_title);
	g_member_info._current_account = p_member_protocal_user_info->_account;
	g_member_info._total_account = get_member_account(g_member_info._level + 1);
	g_member_info._order = p_member_protocal_user_info->_order;
	g_member_info._update_days = get_update_days(g_member_info._current_account, g_member_info._vip_rank);

	/*  将昵称和等级存在配置文件中,以便下次获取失败时可以直接拿来用*/
	sd_snprintf(item_name, 63, "%llu.nickname",p_member_protocal_user_info->_userno);
	em_settings_set_str_item(item_name, g_member_info._nickname);

	sd_snprintf(item_name, 63, "%llu.rank",p_member_protocal_user_info->_userno);
	em_settings_set_int_item(item_name, p_member_protocal_user_info->_rank);
	
	EM_LOG_DEBUG("member_protocal_interface_user_info_resp, username=%s,nickname=%s,userid=%llu,is_new=%d",
		g_member_info._username,g_member_info._nickname,g_member_info._userid,g_member_info._is_new);
	return SUCCESS;
}

_int32 member_protocal_interface_ping_resp(_int32 errcode, MEMBER_PROTOCAL_PING_RESP * p_member_protocal_ping_resp)
{
	if(g_member_data._state != MEMBER_LOGINED_STATE)
		return SUCCESS;
	//这里不去掉定时器，让它继续发送ping
	if(errcode == SUCCESS)
	{
		//成功接收到回复包
		g_member_data._failed = 0;
		if(p_member_protocal_ping_resp->_should_kick == 1)
		{
			//被踢了
			member_remove_login_info(g_member_data._login_info._user_name);
			return member_handle_error(MEMBER_PROTOCAL_ERROR_KICK_OUT);
		}
		if(p_member_protocal_ping_resp->_should_kick == 2)
		{
			//超时需要重连
			member_remove_login_info(g_member_data._login_info._user_name);
			return member_handle_error(MEMBER_PROTOCAL_ERROR_NEED_RETRY);
		}
	}
	/*
	else
	{
		member_handle_error(errcode);
	}
	*/
	return SUCCESS;
}

_int32 member_protocal_interface_report_download_file_resp(_int32 error_code, 
	MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE_RESP *p_member_protocal_rpt_dl_file_resp)
{
	if(error_code != SUCCESS || g_member_data._callback_func == NULL)
		return SUCCESS;
	g_member_info._current_account = p_member_protocal_rpt_dl_file_resp->_total_score;
	//not use
	//g_member_data._callback_func(REFRESH_MEMBER_EVENT, SUCCESS);
	return SUCCESS;
}

_int32 member_protocal_interface_refresh_user_info_resp(MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info)
{
	_u32 nickname_len = 31, username_len = 31;
	char item_name[64] = {0};
	_int32 user_rank = 0,errcode2ui = SUCCESS;

	EM_LOG_DEBUG("member_protocal_impl_query_vip_info_callback,  is_vip = %u", p_member_protocal_user_info->_is_vip);

	//填充数据
	g_member_info._is_vip = p_member_protocal_user_info->_is_vip;
	
	g_member_info._is_year = p_member_protocal_user_info->_is_year;
	
	g_member_info._is_platinum= p_member_protocal_user_info->_is_platinum;

	if(p_member_protocal_user_info->_user_type == 1)
	{
		g_member_info._is_new = FALSE;
		sd_strncpy(g_member_info._username, p_member_protocal_user_info->_user_name, p_member_protocal_user_info->_user_name_len);
	}
	else
	{
		g_member_info._is_new = TRUE;
		sd_u64toa(p_member_protocal_user_info->_user_new_no, g_member_info._username, 32, 10);
	}

	if(p_member_protocal_user_info->_rank==0)
	{
		sd_snprintf(item_name, 63, "%llu.rank",p_member_protocal_user_info->_userno);
		em_settings_get_int_item(item_name, &user_rank);
		p_member_protocal_user_info->_rank = (_u8)user_rank;
	}
	else
	{
		g_member_info._level = p_member_protocal_user_info->_rank;
	}
	g_member_info._vip_rank = p_member_protocal_user_info->_level;
	
	sd_strncpy(g_member_info._expire_date, p_member_protocal_user_info->_expire_date, p_member_protocal_user_info->_expire_date_len);
	
	get_member_military_title(g_member_info._level, g_member_info._military_title);
	g_member_info._current_account = p_member_protocal_user_info->_account;
	g_member_info._total_account = get_member_account(g_member_info._level + 1);
	g_member_info._order = p_member_protocal_user_info->_order;
	g_member_info._update_days = get_update_days(g_member_info._current_account, g_member_info._vip_rank);
;
	// add by hx 2013-9-11
	g_member_info._payid =  p_member_protocal_user_info->_payid;
	sd_strncpy(g_member_info._payname, p_member_protocal_user_info->_payid_name, p_member_protocal_user_info->_payid_name_len);
	// add by hx 2013-10-15
	if(p_member_protocal_user_info->_vas & 0x10000000 )
		g_member_info._is_son_account = TRUE;
	else
		g_member_info._is_son_account = FALSE;

	g_member_info._vas_type = p_member_protocal_user_info->_vas_type;
	
	return SUCCESS;
}


_int32 member_protocal_interface_refresh_resp(_int32 error_code, MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info)
{
	return SUCCESS;
}

_int32 member_protocal_interface_logout_resp(_int32 errcode)
{
	if(g_member_data._state != MEMBER_LOGOUT_STATE)
		return SUCCESS;
//	member_stop_timer();
/*
	if(errcode == SUCCESS)
	{
		g_member_data._state = MEMBER_INIT_STATE;
		g_member_data._callback_func(LOGOUT_SUCCESS_EVENT, SUCCESS);
		g_member_data._failed = 0;
	}
	else
	{
		g_member_data._state = MEMBER_FAILED_STATE;
		g_member_data._callback_func(LOGOUT_FAILED_EVENT, errcode);
		g_member_data._failed = 0;
	}
*/
	return SUCCESS;
}


_int32 member_protocal_interface_query_icon_resp(_int32 errcode, MEMBER_PROTOCAL_ICON * p_icon)
{
	char item_name[64] = {0};
	EM_LOG_DEBUG("member_protocal_interface_query_icon_resp, state=%u,errcode=%d", g_member_data._state,errcode);
	if(errcode != SUCCESS || g_member_data._callback_func == NULL)
		return SUCCESS;
	sd_strncpy(g_member_info._picture_filename, p_icon->_file_path, p_icon->_file_path_len + 1);
	sd_strcat(g_member_info._picture_filename, p_icon->_file_name, p_icon->_file_name_len + 1);
	//g_member_data._callback_func(UPDATE_PICTURE_EVENT, errcode);

	/*  将头像名字存在配置文件中,以便下次获取失败时可以直接拿来用*/
	if(g_member_info._userid!=0)
	{
		sd_snprintf(item_name, 63, "%llu.icon",g_member_info._userid);
		em_settings_set_str_item(item_name, p_icon->_file_name);

		if(!g_icon_callback)
		{
			g_icon_callback = TRUE;
			g_member_data._callback_func(UPDATE_PICTURE_EVENT, errcode);
		}
	}
	
	return SUCCESS;
}

const char* member_get_version(void)
{
	/* 最多31字节 */
	get_version(g_member_data._version, 31);
	return g_member_data._version;
}

_int32 member_save_login_info(const char * account,const char * passwd,MEMBER_INFO * p_member_info)
{
	_int32 ret_val = SUCCESS;
	_u32 tree_id = 0,node_id = 0,cur_time = 0;
	char file_path[64]={0}, cur_time_str[16]={0};

#ifdef KONKA_PROJ 
	/* 康佳项目对非会员不做缓存机制 */
	if(!p_member_info->_is_vip)
	{
		return SUCCESS;
	}
#endif /* KONKA_PROJ */

	sd_snprintf(file_path, 63, "%s.acnt",account);
	EM_LOG_DEBUG("member_save_login_info:%s",file_path);

	/* 删掉旧文件 */
	trm_destroy_tree_impl(file_path);

	/* 重新创建 */
	ret_val = trm_open_tree_impl( file_path,0x1 ,&tree_id);
	CHECK_VALUE(ret_val);

	sd_time(&cur_time);
	sd_snprintf(cur_time_str, 15, "%u",cur_time);

	/* 在picture_filename字段的末尾存放密码 */
	sd_memset(p_member_info->_picture_filename+959,0x00,64);
	sd_strncpy(p_member_info->_picture_filename+960,passwd,62);
	
	ret_val = trm_create_node_impl( tree_id,tree_id,cur_time_str, sd_strlen(cur_time_str),p_member_info,sizeof(MEMBER_INFO),&node_id);

	trm_close_tree_by_id(  tree_id);
	
	if(ret_val!=SUCCESS)
	{
		trm_destroy_tree_impl(file_path);
		CHECK_VALUE(ret_val);
	}
	
	return SUCCESS;
}
_int32 member_load_login_info(const char * account,const char * passwd,MEMBER_INFO * p_member_info)
{
	_int32 ret_val = SUCCESS;
	_u32 tree_id = 0,node_id = 0,create_time = 0, now_time = 0, buffer_len = sizeof(MEMBER_INFO);
	char file_path[64]={0}, create_time_str[16]={0};
	char * passwd_in_cache = NULL;
	
	sd_snprintf(file_path, 63, "%s.acnt",account);
	
	EM_LOG_DEBUG("member_load_login_info:%s",file_path);
	
	ret_val = trm_open_tree_impl( file_path,0x1 ,&tree_id);
	CHECK_VALUE(ret_val);
	
	ret_val = trm_get_first_child_impl( tree_id,tree_id,&node_id);
	if(ret_val!=SUCCESS||node_id==0)
	{
		trm_close_tree_by_id(  tree_id);
		trm_destroy_tree_impl(file_path);
		if(ret_val!=SUCCESS)
		{
			EM_LOG_ERROR("member_load_login_info:ret_val=%d!",ret_val);
			return ret_val;
		}
		else
		{
			EM_LOG_ERROR("member_load_login_info:no info!");
			return -1;
		}
	}

	ret_val = trm_get_data_impl( tree_id, node_id,p_member_info,&buffer_len);
	if(ret_val == SUCCESS)
	{
		ret_val = trm_get_name_impl( tree_id, node_id,create_time_str);
	}
	trm_close_tree_by_id(  tree_id);
	
	if(ret_val!=SUCCESS)
	{
		trm_destroy_tree_impl(file_path);
		CHECK_VALUE(ret_val);
	}
	
	/* 判断密码是否正确 */
	passwd_in_cache = p_member_info->_picture_filename+960;
	if(sd_strlen(passwd_in_cache)==0)
	{
		/* 旧版本,直接返回失败 */
		trm_destroy_tree_impl(file_path);
		return -1;
	}
	else
	if(sd_strcmp(passwd, passwd_in_cache)!=0)
	{
		/* 密码错误 */
		trm_destroy_tree_impl(file_path);
		return MEMBER_PROTOCAL_ERROR_LOGIN_PASSWORD_WRONG;
	}
				

	/* 判断时效 */
	create_time = sd_atoi(create_time_str);
	sd_time(&now_time);
	if(TIME_SUBZ(now_time,create_time)>432000)  /* 最多5天不登录 */
	{
		/* 登录态已过期,不能再用了 */
		//trm_destroy_tree_impl(file_path);
		EM_LOG_ERROR("member_load_login_info:more than 5 day!create_time=%u,now_time=%u!",create_time,now_time);
		//return -1;
	}
	

#if defined(MOBILE_PHONE)
	if(p_member_info->_is_vip && sd_strlen(p_member_info->_expire_date)!=0)
	{
		TIME_t cur_time = {0};
		char t_buffer[16] = {0};
		_int32 expire_year = 0,expire_mon = 0,expire_day = 0;
		ret_val = sd_local_time(&cur_time);
		CHECK_VALUE(ret_val);

		sd_strncpy(t_buffer, p_member_info->_expire_date, 4);
		expire_year = sd_atoi(t_buffer);

		sd_memset(t_buffer,0,16);
		sd_strncpy(t_buffer, p_member_info->_expire_date+4, 2);
		expire_mon = sd_atoi(t_buffer);
		
		sd_memset(t_buffer,0,16);
		sd_strncpy(t_buffer, p_member_info->_expire_date+6, 2);
		expire_day = sd_atoi(t_buffer);

		if((cur_time.year<expire_year)
			||((cur_time.year==expire_year)&&(cur_time.mon<expire_mon))
			||((cur_time.year==expire_year)&&(cur_time.mon==expire_mon)&&(cur_time.mday<=expire_day)))
		{
			return SUCCESS;
		}

		EM_LOG_ERROR("member_load_login_info:expire!expire_time=%d-%d-%d,now_time=%u-%u-%u!",expire_year,expire_mon,expire_day,cur_time.year,cur_time.mon,cur_time.mday);
		/* 登录态已过期,不能再用了 */
		trm_destroy_tree_impl(file_path);
		return -1;
	}
#endif /* MOBILE_PHONE */
	return SUCCESS;
}

BOOL member_has_login_info(const char * account)
{
	BOOL has_info = FALSE;
	char file_path[64]={0};

	sd_snprintf(file_path, 63, "%s.acnt",account);
	
	has_info =  trm_tree_exist_impl(file_path);
	
	EM_LOG_DEBUG("member_has_login_info:file_path =%s, exist_login_info = %d",file_path, has_info);

	return has_info;
}

_int32 member_remove_login_info(const char * account)
{
	_int32 ret_val = SUCCESS;
	char file_path[64]={0};

	sd_snprintf(file_path, 63, "%s.acnt",account);

	if(trm_tree_exist_impl(file_path))
	{
		EM_LOG_DEBUG("member_remove_login_info:%s",file_path);
		/* 删掉缓存文件 */
		ret_val = trm_destroy_tree_impl(file_path);
		CHECK_VALUE(ret_val);
	}
	return SUCCESS;
}

_int32 member_start_test_network(_int32 errcode, _u32 type)
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("member_start_test_network");
#ifdef ENABLE_TEST_NETWORK

	em_set_uid(g_member_info._userid);
	switch(type)
	{
		case 1:
		{
			sd_create_task(em_main_login_server_report_handler, 0, (void*)errcode, NULL);
			break;
		}
		case 2:
		{
			sd_create_task(em_vip_login_server_report_handler, 0, (void*)errcode, NULL);
			break;
		}
		case 3:
		{
			sd_create_task(em_portal_server_report_handler, 0, (void*)errcode, NULL);
			break;
		}
		default:
			break;
	}
#endif

	return SUCCESS;
}

#endif /* ENABLE_MEMBER */

