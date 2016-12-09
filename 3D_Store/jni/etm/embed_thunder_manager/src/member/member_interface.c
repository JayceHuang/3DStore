#include "utility/define.h"

#ifdef ENABLE_MEMBER

#include "member/member_interface.h"
#include "member/member_protocal_interface.h"
#include "member/member_impl.h"
#include "member/member_protocal_impl.h"

#include "utility/md5.h"
#include "utility/string.h"
#include "utility/url.h"
#include "utility/utility.h"

#include "em_common/em_errcode.h"
#include "em_asyn_frame/em_msg.h"
#include "em_asyn_frame/em_notice.h"
#include "em_common/em_logid.h"
#ifdef EM_LOGID
#undef EM_LOGID
#endif
#define EM_LOGID LOGID_MEMBER
#include "em_common/em_logger.h"

static _int32 g_bussiness_type = 0;

_int32 init_member_modular(void)
{
	_int32 ret_val = SUCCESS;

	EM_LOG_DEBUG("init_member_modular");
	
	ret_val = init_member_modular_impl();
	if(ret_val != SUCCESS)
	{
		uninit_member_modular();
	}
	return ret_val;
}

_int32 uninit_member_modular(void)
{
	_int32 ret_val = SUCCESS;
	
	EM_LOG_DEBUG("uninit_member_modular");

	ret_val = uninit_member_modular_impl();
	
	return ret_val;
}

_int32 member_set_bussiness_type(_int32 bussiness_type )
{
	g_bussiness_type = bussiness_type;
	return SUCCESS;
}

_int32 member_get_bussiness_type(void)
{
	return g_bussiness_type;
}

_int32 member_set_callback(void* p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	member_event_notify notify_func = (member_event_notify)_p_param->_para1;

	EM_LOG_DEBUG("member_set_callback");
	
	_p_param->_result = member_set_callback_impl(notify_func);

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));	
}

_int32 member_login(void * p_param)
{
	POST_PARA_3* _p_param = (POST_PARA_3*)p_param;
	char* username_utf8 = (char*)_p_param->_para1;
	char* password = (char*)_p_param->_para2;
	BOOL is_md5 = (BOOL)_p_param->_para3;
	char md5_result[33] = {0};

	EM_LOG_DEBUG("member_login:%s,is_md5=%d",username_utf8,is_md5);

	if(!is_md5)
	{
		encode_password_by_md5(password, sd_strlen(password), md5_result);
		_p_param->_result = member_login_impl(0, username_utf8, sd_strlen(username_utf8), md5_result, sd_strlen(md5_result));
	}
	else
	{
		_p_param->_result = member_login_impl(0, username_utf8, sd_strlen(username_utf8), password, sd_strlen(password));
	}
	
	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 member_sessionid_login(void * p_param)
{
	POST_PARA_2* _p_param = (POST_PARA_2*)p_param;
	char* sessionid = (char*)_p_param->_para1;
	_u64* origin_userid = (_u64*)_p_param->_para2;

	EM_LOG_DEBUG("member_sessionid_login: sessionid = %s, origin_userid = %llu",sessionid, *origin_userid);

	_p_param->_result = member_login_sessionid_impl(sessionid, sd_strlen(sessionid), *origin_userid);

	EM_LOG_DEBUG("member_sessionid_login, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 member_relogin(void * p_param)
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;
	
	EM_LOG_DEBUG("member_relogin");

	_p_param->_result = member_relogin_impl();
	
	EM_LOG_DEBUG("member_relogin, em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}


_int32 member_refresh_user_info(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	member_refresh_notify notify_func = (member_refresh_notify)_p_param->_para1;
	
	_p_param->_result = member_refresh_user_info_impl(notify_func);

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 member_logout(void * p_param)
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;
	
	EM_LOG_DEBUG("member_logout");

	_p_param->_result = member_logout_impl();
	
	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 member_report_download_file(_u64 filesize, _u8 cid[40], char* url, _u32 url_len)
{
	return member_report_download_file_impl(filesize, cid, url, url_len);
}

_int32 member_keepalive_server(void * p_param)
{
	POST_PARA_0* _p_param = (POST_PARA_0*)p_param;
	
	EM_LOG_DEBUG("member_keepalive_server");

	_p_param->_result = member_keepalive();
	
	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 member_get_info(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	MEMBER_INFO* info = (MEMBER_INFO*)_p_param->_para1;

	
	_p_param->_result = get_member_info_impl(info);
	EM_LOG_ERROR("member_get_info:result=%d,userid=%llu,is_vip=%d,is_year=%d,is_platinum=%d,expire_date=%s,is_new=%d,username=%s,nickname=%s",
		_p_param->_result,info->_userid,info->_is_vip,info->_is_year,info->_is_platinum,info->_expire_date,info->_is_new,info->_username,info->_nickname);

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 member_get_picture_filepath(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	char* filepath = (char*)_p_param->_para1;

	EM_LOG_DEBUG("member_get_picture_filepath");
	
	_p_param->_result = get_member_picture_filepath_impl(filepath);

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

BOOL is_member_logined(void)
{
	return is_member_logined_impl();
}

BOOL is_member_logining(void)
{
	return is_member_logining_impl();
}


_int32 member_get_state(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	MEMBER_STATE* state = (MEMBER_STATE*)_p_param->_para1;

	EM_LOG_DEBUG("member_get_state");

	*state = get_member_state_impl();
	
	_p_param->_result = SUCCESS;

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}
/*
const char* get_member_error_info(_int32 errcode)
{
	return member_protocal_interface_error_info(errcode);
}*/

_int32 encode_password_by_md5(char* password, _u32 password_len, char md5_result[33])
{
	_u8 hash[16];
	ctx_md5 md5;
	md5_initialize(&md5);
	md5_update(&md5, (const _u8*)password, password_len);
	md5_finish(&md5, hash);
	str2hex((char*)hash, 16, md5_result, 32);
	md5_result[32] = '\0';
	sd_string_to_low_case(md5_result);
	return 0;
}

_int32 member_register_check_id(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	REGISTER_INFO* reg_info = (REGISTER_INFO*)_p_param->_para1;

	EM_LOG_DEBUG("member_register_check_id");
	_p_param->_result  =  member_register_check_id_impl(reg_info->_username, reg_info->_callback_func, reg_info->_user_data);

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 member_register_user(void * p_param)
{
	POST_PARA_1* _p_param = (POST_PARA_1*)p_param;
	REGISTER_INFO* reg_info = (REGISTER_INFO*)_p_param->_para1;

	EM_LOG_DEBUG("member_register_user");
	_p_param->_result  =  member_register_impl(reg_info->_username, reg_info->_password, reg_info->_nickname, reg_info->_callback_func, reg_info->_user_data, TRUE);

	EM_LOG_DEBUG("em_signal_sevent_handle:_result=%d",_p_param->_result);
	return em_signal_sevent_handle(&(_p_param->_handle));
}

_int32 member_register_cancel(void * p_param)
{
	return NOT_IMPLEMENT;
}
/*
member_login_callback get_member_register_callback()
{
	return member_send_cmd_common_callback_change_handler;
}*/
#else
/* 仅仅是为了在MACOS下能顺利编译 */
#if defined(MACOS)&&(!defined(MOBILE_PHONE))
void member_test(void)
{
	return ;
}
#endif /* defined(MACOS)&&(!defined(MOBILE_PHONE)) */

#endif /* ENABLE_MEMBER */

