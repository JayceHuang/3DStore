#ifndef _MEMBER_PROTOCAL_INTERFACE_H_
#define _MEMBER_PROTOCAL_INTERFACE_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "em_common/em_define.h"

#ifdef ENABLE_MEMBER
#include "member/member_interface.h"
	
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
// 定义错误码

// 未初始化
#define MEMBER_PROTOCAL_ERROR_UNINIT                        (-101)

// 未登录
#define MEMBER_PROTOCAL_ERROR_UNLOGIN                       (-102)

// 无法连上服务器。
#define MEMBER_PROTOCAL_ERROR_SERVER_FAIL                   (-103)

// 服务器返回错误的数据包。
#define MEMBER_PROTOCAL_ERROR_SERVER_UNKNOW_RESP            (-104)

// 发包错误。
#define MEMBER_PROTOCAL_ERROR_SEND_FAIL                     (-105)

// 操作不允许
#define MEMBER_PROTOCAL_ERROR_OPER_DENIED                   (-106)

// 踢人
#define MEMBER_PROTOCAL_ERROR_KICK_OUT                      (-107)

// 超时重连。
#define MEMBER_PROTOCAL_ERROR_NEED_RETRY                    (-108)

//图片路径不存在
#define MEMBER_PROTOCAL_ERROR_PIC_PATH_NOT_EXISTED	(-109)

// 所有login 服务器都连接失败了
#define MEMBER_PROTOCAL_ERROR_SERVER_ALL_FAIL                   (-110)
#define MEMBER_PROTOCAL_ERROR_SEND_LOGIN_FAIL                     (-111)
#define MEMBER_PROTOCAL_ERROR_SEND_LOGIN_PORTAL_FAIL                    (-112)
#define MEMBER_PROTOCAL_ERROR_QUERY_LOGIN_PORTAL_FAIL                    (-113)
#define MEMBER_PROTOCAL_ERROR_SERVER_ALL_VIP_FAIL                   (-114)
#define MEMBER_PROTOCAL_ERROR_NO_SECSSION_ID                   (-115)
#define MEMBER_PROTOCAL_ERROR_QUERY_VIP_PORTAL_FAIL                    (-116)

#define MEMBER_PROTOCAL_ERROR_LOGIN_USE_CACHE                    			(-200)
#define MEMBER_PROTOCAL_ERROR_LOGIN_SESSIONID_UPDATED                    (-201)

// 帐号登录，错误码定义：
#define MEMBER_PROTOCAL_ERROR_LOGIN_ACCOUNT_NOT_EXIST       (2)
#define MEMBER_PROTOCAL_ERROR_LOGIN_PASSWORD_WRONG          (3)
#define MEMBER_PROTOCAL_ERROR_LOGIN_SYSTEM_MAINTENANCE      (6)
#define MEMBER_PROTOCAL_ERROR_LOGIN_ACCOUNT_LOCK            (7)
#define MEMBER_PROTOCAL_ERROR_LOGIN_SERVER_INNER_ERROR      (8)

// ------------------------------------------------------------------------

#define MEMBER_PROTOCAL_USER_NAME_SIZE         (64)
#define MEMBER_PROTOCAL_PASSWD_SIZE            (65)
#define MEMBER_PROTOCAL_PEER_ID_SIZE           (16)
#define MEMBER_PROTOCAL_SESSION_ID_SIZE        (64)
#define MEMBER_PROTOCAL_WEB_SESSION_ID_SIZE   (129)
//#define MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE		(512)
#define MEMBER_PROTOCAL_IMG_URL_SIZE           (128)
#define MEMBER_PROTOCAL_ADDR_SIZE              (32)
#define MEMBER_PROTOCAL_PERSIONAL_SIGN_SIZE    (64)
#define MEMBER_PROTOCAL_PAYID_NAME_SIZE        (64)
#define MEMBER_PROTOCAL_DATE_SIZE              (32)
#define MEMBER_PROTOCAL_FILE_PATH_SIZE         (512)
#define MEMBER_PROTOCAL_FILE_NAME_SIZE         (64)
#define MEMBER_PROTOCAL_CID_SIZE               (40)
#define MEMBER_PROTOCAL_URL_SIZE               (256)

//帐号注册，错误码定义
#define MEMBER_PROTOCAL_ERROR_REGISTER_SUCCESS                                       (200)
#define MEMBER_PROTOCAL_ERROR_REGISTER_ACCOUNT_IS_EXIST                     (300)
#define MEMBER_PROTOCAL_ERROR_REGISTER_PARSE_PROTOCAL_FAILED           (401)
#define MEMBER_PROTOCAL_ERROR_REGISTER_PWD_FORMAT_ERROR                   (411)
#define MEMBER_PROTOCAL_ERROR_REGISTER_ACCOUNT_FORMAT_ERROR           (1011)
#define MEMBER_PROTOCAL_ERROR_REGISTER_CONNECT_NET_FAILED                (1012)
#define MEMBER_PROTOCAL_ERROR_REGISTER_SERVERS_CONMUNICATE_ERROR         (20000)
#define MEMBER_PROTOCAL_ERROR_REGISTER_ERR_NAME_LENGTH                   (20010)
#define MEMBER_PROTOCAL_ERROR_REGISTER_INVALID_NAME                      (20011)
#define MEMBER_PROTOCAL_ERROR_REGISTER_INVALID_CHAR                      (20012)


//登录信息。
typedef struct tag_MEMBER_PROTOCAL_LOGIN_INFO
{
	char _user_name[MEMBER_PROTOCAL_USER_NAME_SIZE];
	_u32 _user_name_len;
	_u32 _login_type;	//	0: 使用username/usernewno/useremail登录	1: 使用userno登录
	_u32 _business_type;
	char _passwd[MEMBER_PROTOCAL_PASSWD_SIZE];	// 未加密。
	_u32 _passwd_len;
	char _peerid[MEMBER_PROTOCAL_PEER_ID_SIZE];
	_u32 _peerid_len;
}MEMBER_PROTOCAL_LOGIN_INFO;

//sessionid登录信息。
typedef struct tag_MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO
{
	char _sessionid[MEMBER_PROTOCAL_WEB_SESSION_ID_SIZE];
	_u32 _sessionid_len;
	_u16 _sessionid_type;	//	0: 页面	1: 客户端 (sessionid的来源类型)
	_u8  _business_type;
	char _peerid[MEMBER_PROTOCAL_PEER_ID_SIZE];
	_u32 _peerid_len;
	_u64 _origin_user_id;
	_u8  _origin_business_type;
}MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO;

// 头像信息。
typedef struct tag_MEMBER_PROTOCAL_ICON
{
	char _file_path[MEMBER_PROTOCAL_FILE_PATH_SIZE];
	_u32 _file_path_len;
	char _file_name[MEMBER_PROTOCAL_FILE_NAME_SIZE];
	_u32 _file_name_len;	// 0: 没有获取到头像。
	_u64 _file_size;
}MEMBER_PROTOCAL_ICON;

// 会员信息。
typedef struct tag_MEMBER_PROTOCAL_USER_INFO
{
	_u64 _userno;	// 用户唯一ID
	char _user_name[MEMBER_PROTOCAL_USER_NAME_SIZE];	// 用户旧字母帐号
	_u32 _user_name_len;
	_u64 _user_new_no;	// 用户新数字帐号
	_u8 _user_type;	// 1：旧帐号  2：新帐号
	char _session_id[MEMBER_PROTOCAL_SESSION_ID_SIZE];
	_u32 _session_id_len;
	char _jumpkey[MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE];
	_u32 _jumpkey_len;
	char _img_url[MEMBER_PROTOCAL_IMG_URL_SIZE];
	_u32 _img_url_len;

	// 用户资料	
	char _nickname[MEMBER_PROTOCAL_USER_NAME_SIZE];
	_u32 _nickname_len;
	_u32 _account;
	_u8 _rank;
	_u32 _order;
	_u8 _is_name2newno;
	char _country[MEMBER_PROTOCAL_ADDR_SIZE];
	_u32 _country_len;
	char _province[MEMBER_PROTOCAL_ADDR_SIZE];
	_u32 _province_len;
	char _city[MEMBER_PROTOCAL_ADDR_SIZE];
	_u32 _city_len;
	_u8 _is_special_num;
	_u8 _role;
	_u32 _today_score;
	_u32 _allow_score;
	char _personal_sign[MEMBER_PROTOCAL_PERSIONAL_SIGN_SIZE];
	_u32 _personal_sign_len;

	// vip 相关信息
	_u8 _is_vip;	// 是否VIP
	_u8 _is_year;  // 是否年费会员
	_u32 _is_platinum; //是否白金会员
	_u32 _vas;	// (可不管)
	_u16 _level;	// VIP等级(由VIP成长值算得)
	_u32 _grow;	// VIP成长值
	_u32 _payid;	// VIP支付类型(须注意以下几种类型, 5:"年费"， 3:"手机包月",  >1000:"体验会员" )
	char _payid_name[MEMBER_PROTOCAL_PAYID_NAME_SIZE];	// VIP支付类型名称
	_u32 _payid_name_len;
	char _expire_date[MEMBER_PROTOCAL_DATE_SIZE];	// 	过期日期
	_u32 _expire_date_len;
	_u8 _is_auto_deduct;	// 是否自动续费(可不管)
	_u8 _is_remind;	// 是否提醒(可不管)
	_u32 _daily;	  // VIP每日成长值(由支付类型决定，其中，年费:15/日， 手机:5/日， 体验:0/日。其余的5<daily<15。)
	_u32 _vas_type; //会员类型: 2-普通会员 3-白金会员 4--钻石会员
	// 头像
	MEMBER_PROTOCAL_ICON _icon;
}MEMBER_PROTOCAL_USER_INFO;

// 在线信息。
typedef struct tag_MEMBER_PROTOCAL_PING
{
	_u64 _userno;
	char _session_id[MEMBER_PROTOCAL_SESSION_ID_SIZE];
	_u32 _session_id_len;
}MEMBER_PROTOCAL_PING;

// 在线信息应答。
typedef struct tag_MEMBER_PROTOCAL_PING_RESP
{
	_u64 _userno;
	_u8 _should_kick;	//	0：不作处理; 1：踢人（不能重连）; 2：超时（需要重连）
}MEMBER_PROTOCAL_PING_RESP;

// 上报下载文件。
typedef struct tag_MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE
{
	_u64 _userno;
	char _session_id[MEMBER_PROTOCAL_SESSION_ID_SIZE];
	_u32 _session_id_len;
	_u64 _dl_file_size;
	char _cid[MEMBER_PROTOCAL_CID_SIZE];
	_u32 _cid_len;
	char _url[MEMBER_PROTOCAL_URL_SIZE];
	_u32 _url_len;
	_u8 _dead_link;
	_u32 _saved_time;
}MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE;

// 上报下载文件应答。
typedef struct tag_MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE_RESP
{
	_u32 _today_score;	// 当天得分（在线时间积分+文件积分）
	_u32 _df_score;	// 本次下载得分
	_u32 _total_score;	// 用户总分（历史以来的总分）
	_u32 _allow_score;	// 当天还允许增加分数
}MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE_RESP;

// 注销信息。
typedef struct tag_MEMBER_PROTOCAL_LOGOUT_INFO
{
	_u64 _userno;
	char _session_id[MEMBER_PROTOCAL_SESSION_ID_SIZE];
	_u32 _session_id_len;
}MEMBER_PROTOCAL_LOGOUT_INFO;

//检查user name信息
typedef struct tagMEMBER_PROTOCAL_REGISTER_CHECK_ID
{
	void* _user_data;
	member_register_callback _callback_func;
	char _userid[MEMBER_PROTOCAL_USER_NAME_SIZE];
	_int32 _usertype;
}MEMBER_PROTOCAL_REGISTER_CHECK_ID;

//注册信息
typedef struct tagMEMBER_PROTOCAL_REGISTER_INFO
{	
	void         *_user_data;
	member_register_callback _callback_func;

	char         _userid[MEMBER_PROTOCAL_USER_NAME_SIZE];
	_int32       _usertype;										//用户类型0::旧帐号,1:新帐号，默认为0
	char         _password[MEMBER_PROTOCAL_PASSWD_SIZE];
	_int32       _registertype;									//注册类型0:网游注册,1:迅雷注册,2:ipad
	char         _nickname[MEMBER_PROTOCAL_USER_NAME_SIZE];
}MEMBER_PROTOCAL_REGISTER_INFO;

typedef struct tagMEMBER_PROTOCAL_REGISTER_RESULT_INFO
{
	char        _command_id[64];
	_u32       _result;
}MEMBER_PROTOCAL_REGISTER_RESULT_INFO;

// ------------------------------------------------------------------------

//获取错误码解释。
const char * member_protocal_interface_error_info(_int32 error_code);

// ------------------------------------------------------------------------

// 初始化。
_int32 member_protocal_interface_init(char *file_path, _u32 file_path_len);

// 反初始化。
_int32 member_protocal_interface_uninit(void);

// 用户登录。
_int32 member_protocal_interface_first_login(MEMBER_PROTOCAL_LOGIN_INFO * p_member_protocal_login_info);
_int32 member_protocal_interface_login(MEMBER_PROTOCAL_LOGIN_INFO * p_member_protocal_login_info);

// sessionid登录。
_int32 member_protocal_interface_sessionid_login(MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO * p_member_protocal_sessionid_login_info);

// 用户登录时的基础数据回调
_int32 member_protocal_interface_login_basic_resp( MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info);

// 用户登录回调。
// error_code == SUCCESS, 成功。
// error_code != SUCCESS, 失败，此时 p_member_protocal_user_info 为NULL。
// 虚函数，必须由调用模块实现定义。
_int32 member_protocal_interface_login_resp(_int32 error_code, MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info);

// 用户上报在线信息。
_int32 member_protocal_interface_ping(MEMBER_PROTOCAL_PING * p_member_protocal_ping);

// 用户上报在线信息回调。
// error_code == SUCCESS, 成功。
// error_code != SUCCESS, 失败，此时 p_member_protocal_ping_resp 为NULL。
// 虚函数，必须由调用模块实现定义。
_int32 member_protocal_interface_ping_resp(_int32 error_code, MEMBER_PROTOCAL_PING_RESP *p_member_protocal_ping_resp);

// 上报下载文件。
_int32 member_protocal_interface_report_download_file(MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE * p_member_protocal_rpt_dl_file);

// 上报下载文件回调。
// error_code == SUCCESS, 成功。
// error_code != SUCCESS, 失败，此时 p_member_protocal_ping_resp 为NULL。
// 虚函数，必须由调用模块实现定义。
_int32 member_protocal_interface_report_download_file_resp(_int32 error_code, 
	MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE_RESP *p_member_protocal_rpt_dl_file_resp);

// 用户信息刷新。
_int32 member_protocal_interface_refresh(MEMBER_PROTOCAL_LOGIN_INFO * p_member_protocal_login_info);

// 用户信息刷新回调。
// error_code == SUCCESS, 成功。
// error_code != SUCCESS, 失败，此时 p_member_protocal_user_info 为NULL。
// 虚函数，必须由调用模块实现定义。
_int32 member_protocal_interface_refresh_resp(_int32 error_code, MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info);

// 用户注销。
_int32 member_protocal_interface_logout(MEMBER_PROTOCAL_LOGOUT_INFO * p_member_protocal_logout_info);

_int32 member_protocal_interface_change_login_type(_u32 type);

//检查用户ID
_int32 member_protocal_interface_register_check_id(MEMBER_PROTOCAL_REGISTER_CHECK_ID*check_id);

//注册用户
_int32 member_protocal_interface_register(MEMBER_PROTOCAL_REGISTER_INFO *register_info, BOOL is_utf8);

// 用户注销回调。
// error_code == SUCCESS, 成功。
// error_code != SUCCESS, 失败。
// 虚函数，必须由调用模块实现定义。
_int32 member_protocal_interface_logout_resp(_int32 error_code);

// 查询头像回调。
// error_code == SUCCESS, 成功。
// error_code != SUCCESS, 失败，此时 p_icon 为NULL。
// 虚函数，必须由调用模块实现定义。
_int32 member_protocal_interface_query_icon_resp(_int32 error_code, MEMBER_PROTOCAL_ICON * p_icon);

/* user info 有可能在login之后才获得 */
_int32 member_protocal_interface_user_info_resp(_int32 errcode, MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info);

_int32 member_protocal_interface_refresh_user_info(member_refresh_notify func);

// ------------------------------------------------------------------------
#endif /* ENABLE_MEMBER */

#ifdef __cplusplus
}
#endif

#endif

