#ifndef _MEMBER_PROTOCAL_IMPL_H_
#define _MEMBER_PROTOCAL_IMPL_H_

#ifdef __cplusplus
extern "C" 
{
#endif

// ------------------------------------------------------------------------
#include "em_common/em_define.h"

#ifdef ENABLE_MEMBER
#include "member/member_protocal_interface.h"

// ------------------------------------------------------------------------
//login_vip timeout
#define MAX_VIP_LOGIN_TIMEOUT  (8)

// login server
#define MEMBER_SERVER_MAX_COUNT (16)

// 当前状态
#define MEMBER_STATE_UNLOGIN                (0)
#define MEMBER_STATE_LOGINING               (1)
#define MEMBER_STATE_LOGINED                (2)
#define MEMBER_STATE_PING                   (3)
#define MEMBER_STATE_REPORT_DOWNLOAD_FILE   (4)
#define MEMBER_STATE_REFRESHING             (5)

// 当前异步操作类型
#define MEMBER_ASYN_OPER_NONE              (0)
#define MEMBER_ASYN_OPER_LOGIN             (1)
#define MEMBER_ASYN_OPER_QUERY_USER_INFO   (2)
#define MEMBER_ASYN_OPER_QUERY_PORTAL      (3)
#define MEMBER_ASYN_OPER_LOGIN_VIP         (4)
#define MEMBER_ASYN_OPER_QUERY_PORTAL_VIP  (5)
#define MEMBER_ASYN_OPER_PING              (6)
#define MEMBER_ASYN_OPER_LOGOUT            (7)
#define MEMBER_ASYN_OPER_REFRESH           (8)


#define XML_NODE_TYPE_COMMAND                 (1001)


// server信息。
typedef struct tag_MEMBER_SERVER_INFO
{
	_u32 _ip;
	_u16 _port;
	_u16 _protocal;	// 0-tcp, 1-udp
}MEMBER_SERVER_INFO;

typedef struct tag_MEMBER_PROTOCAL MEMBER_PROTOCAL_IMPL;

typedef _int32 ( *DEAL_WITH_LOGIN_RESULT_CALLBACK)(_int32 error_code, 
	MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info, MEMBER_PROTOCAL_IMPL * p_member);

struct tag_MEMBER_PROTOCAL
{
	// 主服务器域名/port.
	char _main_server_host[MAX_HOST_NAME_LEN];
	_u32 _main_server_host_len;
	_u16 _main_server_port;

	// portal服务器域名/port.
	char _portal_server_host[MAX_HOST_NAME_LEN];
	_u32 _portal_server_host_len;
	_u16 _portal_server_port;

	// 服务器IP地址。
	_u32 _server_count;	// server 的总数.
	_u32 _server_index;	// 可用的server index.
	MEMBER_SERVER_INFO _servers[MEMBER_SERVER_MAX_COUNT];
	MEMBER_SERVER_INFO _portal_server;

	// vip 主服务器域名/port.
	char _vip_main_server_host[MAX_HOST_NAME_LEN];
	_u32 _vip_main_server_host_len;
	_u16 _vip_main_server_port;

	// vip portal服务器域名/port.
	char _vip_portal_server_host[MAX_HOST_NAME_LEN];
	_u32 _vip_portal_server_host_len;
	_u16 _vip_portal_server_port;

	// vip 服务器IP地址。
	_u32 _vip_server_count;	// vip_server 的总数.
	_u32 _vip_server_index;	// 可用的 vip_server index.
	MEMBER_SERVER_INFO _vip_servers[MEMBER_SERVER_MAX_COUNT];
	MEMBER_SERVER_INFO _vip_portal_server;

	// 状态
	_u32 _state;
	
	// 收发超时
	_u32 _asyn_time_out;	// 单位: 秒
	DEAL_WITH_LOGIN_RESULT_CALLBACK _login_result_callback;
		
	MEMBER_PROTOCAL_LOGIN_INFO _login_info;	// 重试或刷新信息时备用。
	MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO _session_login_info;
	MEMBER_PROTOCAL_USER_INFO _user_info;	// 分布查询用户信息。
};

// ------------------------------------------------------------------------

// 初始化。
_int32 member_protocal_impl_init(MEMBER_PROTOCAL_IMPL *p_member, char *file_path, _u32 file_path_len,
	_u32 time_out);

// 反初始化。
_int32 member_protocal_impl_uninit(MEMBER_PROTOCAL_IMPL *p_member);

_int32 member_protocal_impl_change_login_flag(_u32 new_flag);

_int32 member_protocal_impl_state_change(MEMBER_PROTOCAL_IMPL * p_member, _u32 new_state);

_int32 member_protocal_impl_first_login(MEMBER_PROTOCAL_IMPL * p_member);
_int32 member_protocal_impl_login(MEMBER_PROTOCAL_IMPL * p_member);
_int32 member_protocal_impl_login_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_sessionid_login(MEMBER_PROTOCAL_IMPL * p_member);
_int32 member_protocal_impl_sessionid_login_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_query_user_info(MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_impl_query_user_info_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_query_portal(MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_impl_query_portal_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_login_vip(MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_impl_login_vip_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_query_portal_vip(MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_impl_query_portal_vip_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_ping(MEMBER_PROTOCAL_IMPL * p_member, MEMBER_PROTOCAL_PING * p_member_protocal_ping);

_int32 member_protocal_impl_ping_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_report_download_file(MEMBER_PROTOCAL_IMPL * p_member,
	MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE * p_member_protocal_rpt_dl_file);

_int32 member_protocal_impl_report_download_file_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_refresh(MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_impl_refresh_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_query_vip_info(MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_impl_query_vip_info_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_logout(MEMBER_PROTOCAL_IMPL * p_member, MEMBER_PROTOCAL_LOGOUT_INFO * p_member_protocal_logout_info);

_int32 member_protocal_impl_logout_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_query_icon(MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_impl_query_icon_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_register_query(MEMBER_PROTOCAL_REGISTER_INFO *register_info, BOOL is_utf8);

_int32 member_protocal_impl_register_query_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

_int32 member_protocal_impl_register_check_id_query(MEMBER_PROTOCAL_REGISTER_CHECK_ID *check_id);

_int32 member_protocal_impl_register_check_id_query_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

// 接口回调处理函数
_int32 member_protocal_interface_login_basic_resp_impl(MEMBER_PROTOCAL_USER_INFO * p_user_info);

_int32 member_protocal_interface_login_resp_impl(_int32 error_code, 
	MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info, MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_interface_sessionid_login_resp_impl(_int32 error_code, 
	MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info, MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_interface_ping_resp_impl(_int32 error_code,
	MEMBER_PROTOCAL_PING_RESP *p_member_protocal_ping_resp, MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_interface_refresh_resp_impl(_int32 error_code,
	MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info, MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_interface_logout_resp_impl(_int32 error_code, MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_interface_query_icon_resp_impl(_int32 error_code, 
	MEMBER_PROTOCAL_ICON * p_icon, MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_protocal_interface_report_download_file_resp_impl(_int32 error_code, 
	MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE_RESP * p_rpt_dl_file_resp, MEMBER_PROTOCAL_IMPL * p_member);

_int32 member_send_cmd_common_callback(void * user_data, _int32 result,
									   void * recv_buffer, _u32 recvd_len, void * p_header,_u8 * send_data);

//_int32 member_send_cmd_by_url_common_callback(ETM_HTTP_CALL_BACK * p_http_call_back_param);
	
_int32 member_send_cmd_common_callback_change_handler(void *param);
// ------------------------------------------------------------------------

_int32 member_protocal_impl_refresh_user_info(MEMBER_PROTOCAL_IMPL * p_member, member_refresh_notify func);

#endif /* ENABLE_MEMBER */
#ifdef __cplusplus
}
#endif

#endif

