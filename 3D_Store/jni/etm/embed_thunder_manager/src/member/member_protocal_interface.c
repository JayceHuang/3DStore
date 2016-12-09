#include "utility/define.h"

#ifdef ENABLE_MEMBER

#include "member/member_protocal_interface.h"
#include "member/member_protocal_impl.h"

#include "utility/string.h"

#include "em_common/em_define.h"
#include "em_common/em_errcode.h"
// ------------------------------------------------------------------------

static MEMBER_PROTOCAL_IMPL g_member;

// ------------------------------------------------------------------------

const char * member_protocal_interface_error_info(_int32 error_code)
{
	switch (error_code)
	{
	case MEMBER_PROTOCAL_ERROR_UNINIT:
		{
			static const char *err_str = "初始化失败";
			return err_str;
		}
		break;
	case MEMBER_PROTOCAL_ERROR_UNLOGIN:
		{
			static const char *err_str = "帐号未登录";
			return err_str;
		}
		break;
	case MEMBER_PROTOCAL_ERROR_LOGIN_ACCOUNT_NOT_EXIST:
	case MEMBER_PROTOCAL_ERROR_LOGIN_PASSWORD_WRONG:
		{
			static const char *err_str = "帐号或密码错误";
			return err_str;
		}
		break;
	case MEMBER_PROTOCAL_ERROR_LOGIN_ACCOUNT_LOCK:
		{
			static const char *err_str = "帐号已锁定";
			return err_str;
		}
		break;
	case MEMBER_PROTOCAL_ERROR_KICK_OUT:
		{
			static const char *err_str = "帐号在另一地点登录,您将被迫下线!";
			return err_str;
		}
		break;
	case MEMBER_PROTOCAL_ERROR_NEED_RETRY:
		{
			static const char *err_str = "网络连接超时,请重新登录";
			return err_str;
		}
		break;
	case MEMBER_PROTOCAL_ERROR_OPER_DENIED:
		{
			static const char *err_str = "本次操作不允许";
			return err_str;
		}
		break;
	case MEMBER_PROTOCAL_ERROR_REGISTER_ACCOUNT_IS_EXIST:
		{
			static const char *err_str = "该帐号已经存在!";
			return err_str;
		}
		break;
	case MEMBER_PROTOCAL_ERROR_REGISTER_PARSE_PROTOCAL_FAILED:
		{
			static const char *err_str = "协议解析失败!";
			return err_str;
		}
		break;
	case MEMBER_PROTOCAL_ERROR_REGISTER_PWD_FORMAT_ERROR:
		{
			static const char *err_str = "密码格式错误!";
			return err_str;			
		}
		break;
	case MEMBER_PROTOCAL_ERROR_REGISTER_ACCOUNT_FORMAT_ERROR:
	case MEMBER_PROTOCAL_ERROR_REGISTER_INVALID_NAME:
	case MEMBER_PROTOCAL_ERROR_REGISTER_INVALID_CHAR:
		{
			static const char *err_str = "用户名必须(且只能)由字母和数字组成!";
			return err_str;			
		}
		break;
	case MEMBER_PROTOCAL_ERROR_REGISTER_ERR_NAME_LENGTH:
		{
			static const char *err_str = "用户名长度必须在6-16以内!";
			return err_str;
		}
		break;
	case MEMBER_PROTOCAL_ERROR_REGISTER_CONNECT_NET_FAILED:
	case MEMBER_PROTOCAL_ERROR_REGISTER_SERVERS_CONMUNICATE_ERROR:
	default:
		{
			static const char *err_str = "连接服务器失败";
			return err_str;
		}
		break;
	}
	return NULL;
}

// ------------------------------------------------------------------------

_int32 member_protocal_interface_init(char *file_path, _u32 file_path_len)
{
	_int32 ret = SUCCESS;
	ret = member_protocal_impl_init(&g_member, file_path, file_path_len, MEMBER_DEFAULT_LOGIN_TIMEOUT);
	return ret;
}

_int32 member_protocal_interface_uninit(void)
{
	_int32 ret = SUCCESS;
	ret = member_protocal_impl_uninit(&g_member);
	return ret;
}

// 用户登录。
_int32 member_protocal_interface_first_login(MEMBER_PROTOCAL_LOGIN_INFO * p_member_protocal_login_info)
{
	_int32 ret = SUCCESS;
	sd_memcpy(&g_member._login_info, p_member_protocal_login_info, sizeof(MEMBER_PROTOCAL_LOGIN_INFO));
	g_member._login_result_callback = member_protocal_interface_login_resp_impl;
	ret = member_protocal_impl_first_login(&g_member);
	return ret;
}
_int32 member_protocal_interface_login(MEMBER_PROTOCAL_LOGIN_INFO * p_member_protocal_login_info)
{
	_int32 ret = SUCCESS;
	sd_memcpy(&g_member._login_info, p_member_protocal_login_info, sizeof(MEMBER_PROTOCAL_LOGIN_INFO));
	g_member._login_result_callback = member_protocal_interface_login_resp_impl;
	ret = member_protocal_impl_login(&g_member);
	return ret;
}

_int32 member_protocal_interface_sessionid_login(MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO * p_member_protocal_sessionid_login_info)
{
	_int32 ret = SUCCESS;
	sd_memcpy(&g_member._session_login_info, p_member_protocal_sessionid_login_info, sizeof(MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO));
	g_member._login_result_callback = member_protocal_interface_sessionid_login_resp_impl;
	ret = member_protocal_impl_sessionid_login(&g_member);
	return ret;
}

// 用户上报在线信息。
_int32 member_protocal_interface_ping(MEMBER_PROTOCAL_PING * p_member_protocal_ping)
{
	_int32 ret = SUCCESS;
	ret = member_protocal_impl_ping(&g_member, p_member_protocal_ping);
	return ret;
}

// 上报下载文件。
_int32 member_protocal_interface_report_download_file(MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE * p_member_protocal_rpt_dl_file)
{
	_int32 ret = SUCCESS;
	ret = member_protocal_impl_report_download_file(&g_member, p_member_protocal_rpt_dl_file);
	return ret;
}

// 用户信息刷新。
_int32 member_protocal_interface_refresh(MEMBER_PROTOCAL_LOGIN_INFO * p_member_protocal_login_info)
{
	_int32 ret = SUCCESS;
	sd_memcpy(&g_member._login_info, p_member_protocal_login_info, sizeof(MEMBER_PROTOCAL_LOGIN_INFO));
	ret = member_protocal_impl_refresh(&g_member);
	return ret;
}

_int32 member_protocal_interface_refresh_user_info(member_refresh_notify func)
{
	_int32 ret = SUCCESS;
	
	ret = member_protocal_impl_refresh_user_info(&g_member, func);
	return ret;
}


// 用户注销。
_int32 member_protocal_interface_logout(MEMBER_PROTOCAL_LOGOUT_INFO * p_member_protocal_logout_info)
{
	_int32 ret = SUCCESS;
    // clear member userinfo
    sd_memset(&g_member._user_info, 0, sizeof(MEMBER_PROTOCAL_USER_INFO));
	ret = member_protocal_impl_logout(&g_member, p_member_protocal_logout_info);
	return ret;
}

_int32 member_protocal_interface_change_login_type(_u32 type)
{
	_int32 ret = SUCCESS;

	ret = member_protocal_impl_change_login_flag(type);
	return ret;
}

//查询用户ID
_int32 member_protocal_interface_register_check_id(MEMBER_PROTOCAL_REGISTER_CHECK_ID*check_id)
{
	return member_protocal_impl_register_check_id_query(check_id);
}

//注册用户
_int32 member_protocal_interface_register(MEMBER_PROTOCAL_REGISTER_INFO *register_info, BOOL is_utf8)
{
	return member_protocal_impl_register_query(register_info, is_utf8);
}

// ------------------------------------------------------------------------
#endif /* ENABLE_MEMBER */

