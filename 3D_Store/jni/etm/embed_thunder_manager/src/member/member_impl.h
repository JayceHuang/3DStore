/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2010/05/19
-----------------------------------------------------------------------------------------------------------*/
#ifndef _MEMBER_IMPL_H_
#define _MEMBER_IMPL_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"

#ifdef ENABLE_MEMBER
#include "member/member_interface.h"
#include "member/member_protocal_interface.h"

#define	MEMBER_INVALID_TIMER_ID			0
#define	MEMBER_MAX_FAILED_COUNT			3

#define		E_MEMBER_SEND_RESP			(E_USER + 999)

typedef struct tagMEMBER_DATA
{
	_int32				_timer_id;
	_u32				_failed;			//失败次数
	_u32				_type;
	char				_version[MAX_VERSION_LEN];	//版本号
	_u32				_state;
	member_event_notify	_callback_func;
	MEMBER_PROTOCAL_LOGIN_INFO _login_info;
	MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO _sessionid_login_info;
	MEMBER_PROTOCAL_PING		 _ping_info;
}MEMBER_DATA;

_int32 init_member_modular_impl(void);

_int32 uninit_member_modular_impl(void);

_int32 member_set_callback_impl(member_event_notify callback_func);

_int32 member_login_impl(_u32 type, char* username, _u32 username_len, char* pwd, _u32 pwd_len);

_int32 member_login_sessionid_impl(char* sessionid,  _u32 sessionid_len, _u64 origin_user_id);

_int32 member_protocal_interface_sessionid_login_resp(_int32 errcode, MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info);

_int32 member_relogin_impl(void);

_int32 member_logout_impl(void);

_int32 member_report_download_file_impl(_u64 filesize, _u8 cid[40], char* url, _u32 url_len);

_int32 member_register_check_id_impl(char *user_name, member_register_callback callback_func, void *user_data);

_int32 member_register_impl(char *user_name, char *password, char *nickname,
	member_register_callback callback_func, void *user_data, BOOL is_utf8);

_int32 get_member_info_impl(MEMBER_INFO* info);

_int32 get_member_picture_filepath_impl(char* filepath);

BOOL is_member_logined_impl(void);

BOOL is_member_logining_impl(void);

MEMBER_STATE get_member_state_impl(void);

_int32 member_handle_error(_int32 errcode);

_int32 member_keepalive(void);

const char* member_get_version(void);

/* 增加登录态本地存储机制 by zyq @ 20130107 */
_int32 member_save_login_info(const char * account,const char * passwd,MEMBER_INFO * p_member_info);
_int32 member_load_login_info(const char * account,const char * passwd,MEMBER_INFO * p_member_info);
BOOL member_has_login_info(const char * account);
_int32 member_remove_login_info(const char * account);

_int32 member_start_test_network(_int32 errcode, _u32 type);

_int32 member_refresh_user_info_impl(member_refresh_notify func);
_int32 member_protocal_interface_refresh_user_info_resp(MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info);

#endif /* ENABLE_MEMBER */

#ifdef __cplusplus
}
#endif

#endif
