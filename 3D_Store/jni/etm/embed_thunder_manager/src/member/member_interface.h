/*----------------------------------------------------------------------------------------------------------
author:	ZHANGSHAOHAN
created:	2010/05/19
-----------------------------------------------------------------------------------------------------------*/
#ifndef _MEMBER_INTERFACE_H_
#define _MEMBER_INTERFACE_H_


#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"

#ifdef ENABLE_MEMBER

/* 登录事件 */
typedef enum tag_MEMBER_EVENT
{
	LOGIN_LOGINED_EVENT = 0,
	LOGIN_FAILED_EVENT,
//	LOGOUT_SUCCESS_EVENT,
//	LOGOUT_FAILED_EVENT,
//	PING_FAILED_EVENT,
	UPDATE_PICTURE_EVENT,
	KICK_OUT_EVENT,			//账户在别处登录，被踢了
	NEED_RELOGIN_EVENT		//账户需要重新登录
//	MEMBER_LOGOUT_EVENT,
//	REFRESH_MEMBER_EVENT,
//	MEMBER_CANCEL_EVENT
}MEMBER_EVENT;
	
#define MEMBER_SESSION_ID_SIZE 64
#define MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE 512
#define MEMBER_MAX_USERNAME_LEN 64
#define MEMBER_MAX_PASSWORD_LEN 64
#define MEMBER_MAX_DATE_SIZE 32
#define MEMBER_MAX_PAYID_NAME_SIZE 64
/* 登录信息 */
typedef struct tagMEMBER_INFO
{
	BOOL _is_vip;		//是否会员
	BOOL _is_year;		//是否年费会员
	BOOL _is_platinum;	//是否白金礼包用户
	_u64 _userid;		//全局唯一id
	BOOL _is_new;		//username是否新帐号，新帐号username为纯数字
	char _username[MEMBER_MAX_USERNAME_LEN];		//账户名
	char _nickname[MEMBER_MAX_USERNAME_LEN];		//用户昵称
	char _military_title[16];							//军衔
	_u16 _level;
	_u16 _vip_rank;
	char _expire_date[MEMBER_MAX_DATE_SIZE];		// VIP过期日期
	_u32 _current_account;
	_u32 _total_account;
	_u32 _order;
	_u32 _update_days;
	char _picture_filename[1024];
	char _session_id[MEMBER_SESSION_ID_SIZE];
	_u32 _session_id_len;
	char _jumpkey[MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE];
	_u32 _jumpkey_len;
	_u32 _payid;
	char _payname[MEMBER_MAX_PAYID_NAME_SIZE];
	BOOL _is_son_account;		//是否企业子账号
	_u32 _vas_type;	          //会员类型: 2-普通会员 3-白金会员 4--钻石会员
}MEMBER_INFO;

/* 登录状态 */
typedef enum tagMEMBER_STATE
{
	MEMBER_INIT_STATE = 0,
	MEMBER_LOGINING_STATE,		//正在登陆的状态
	MEMBER_LOGINED_STATE,		//登陆成功的状态
	MEMBER_LOGOUT_STATE,		//正在退出登陆的状态
	MEMBER_FAILED_STATE
}MEMBER_STATE;

typedef _int32 (*member_event_notify)(MEMBER_EVENT event, _int32 errcode);

typedef _int32 (*member_register_callback)(_int32 errcode, void *user_data);

typedef _int32 (*member_refresh_notify)(_int32 result);

typedef struct tagREGISTER_INFO
{
	char _username[MEMBER_MAX_USERNAME_LEN];
	char _password[MEMBER_MAX_PASSWORD_LEN];
	char _nickname[MEMBER_MAX_USERNAME_LEN];
	void* _user_data;
	member_register_callback _callback_func;
}REGISTER_INFO;

_int32 init_member_modular(void);

_int32 uninit_member_modular(void);

_int32 member_set_bussiness_type(_int32 bussiness_type );
_int32 member_get_bussiness_type(void);
#define MEMBER_BUSSINESS_TYPE member_get_bussiness_type()

_int32 member_set_callback(void* p_param);

_int32 member_login(void* p_param);

_int32 member_sessionid_login(void * p_param);

_int32 member_relogin(void * p_param);

_int32 member_refresh_user_info(void * p_param);

_int32 member_logout(void* p_param);

_int32 member_report_download_file(_u64 filesize, _u8 cid[40], char* url, _u32 url_len);

_int32 member_keepalive_server(void * p_param);

//如果没有登录成功调用该函数会返回失败
_int32 member_get_info(void* p_param);

_int32 member_get_picture_filepath(void* p_param);

BOOL is_member_logined(void);

BOOL is_member_logining(void);

_int32 member_get_state(void* p_param);

//const char* get_member_error_info(_int32 errcode);

_int32 encode_password_by_md5(char* password, _u32 password_len, char md5_result[33]);

_int32 member_register_check_id(void * p_param);

_int32 member_register_user(void * p_param);
	
//member_login_callback get_member_register_callback();
	
_int32 member_register_cancel(void * p_param);

#endif /* ENABLE_MEMBER */

#ifdef __cplusplus
}
#endif

#endif
