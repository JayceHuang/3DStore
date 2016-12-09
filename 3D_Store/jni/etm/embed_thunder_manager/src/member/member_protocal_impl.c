#include "utility/define.h"

#ifdef ENABLE_MEMBER

//#include "event.h"
#include "member/member_protocal_impl.h"
#include "member/member_utility.h"
#include "member/member_impl.h"

#include "utility/utility.h"
#include "utility/md5.h"
#include "utility/base64.h"
#include "utility/bytebuffer.h"
#include "utility/string.h"
#include "utility/sd_iconv.h"
#include "utility/mempool.h"
#include "utility/time.h"
#include "utility/logid.h"

#include "platform/sd_fs.h"
#include "download_manager/mini_task_interface.h"
#include "scheduler/scheduler.h"

#ifdef ENABLE_XML
#include "xml_service.h"
#endif /* ENABLE_XML  */

#define LOGID LOGID_MEMBER
#include "utility/logger.h"

#if defined(__SYMBIAN32__) || defined(MACOS)
#else
#define SEND_CALLBACK_NOT_IN_THUNDER_INTERFACE_THREAD
#endif

#define QUERY_VIP_PROTOCOL_VERSION 110

#define CMD_ID_SESSIONID_LOGIN 0x0031
// ------------------------------------------------------------------------

// 临时的收发buffer.
//#define TMP_DATA_SIZE (1024)
//static char g_tmp_data[TMP_DATA_SIZE];

#define TMP_DATA_SIZE (16 * 1024)
static char *g_tmp_data = NULL;
static char *g_tmp_data_vip = NULL;
static char *g_tmp_data_portal = NULL;

static _int32 g_login_state = 0;  	/* 0:未登录,1:正在登录 ,2: 登录成功,3:登录失败*/
static _int32 g_login_vip_state = 0;  /* 0:未登录,1:正在登录 ,2: 登录成功,3:登录失败*/
static BOOL g_querying_portal = FALSE;  /* 正在查询portal */
static  BOOL g_querying_vip_portal = FALSE;  /*  正在查询vip portal  */
static _u32 g_login_start_time_stamp = 0;
static _u32 g_login_max_time_out = 0;

member_refresh_notify g_refresh_user_info_callback;

#ifdef ENABLE_TCP_LOGIN

#include "interface/sd_socket.h"
#include <netdb.h>
#include <sys/socket.h>

static _int32 g_http_login_state = 0;  	/* 0:http登录流程， 1: tcp登录流程*/
#endif

#ifdef ENABLE_LOGIN_REPORT

#define REPORT_SERVER_IP    DEFAULT_LOCAL_IP   //"stat.login.xunlei.com"
#define REPORT_SERVER_PORT  1800

static _u32 g_login_all_time = 0;
static _u32 g_login_query_vip_info = 0;

_int32 member_report_login_info_resp(EM_HTTP_CALL_BACK* param)
{
	LOG_DEBUG("member_report_login_info_resp, param->_type = %d, result = %d", param->_type, param->_result);

    switch (param->_type)
    {
        case EMHC_NOTIFY_RESPN:
        {
            _u32 http_status_code = *((_u32*)param->_header);
            break;   
        }
        case EMHC_GET_SEND_DATA:
        {
            break;
        }
        case EMHC_NOTIFY_SENT_DATA:
        {
            break;
        }
        case EMHC_GET_RECV_BUFFER:
        {
            break;
        }
        case EMHC_PUT_RECVED_DATA:
        {
            if( NULL != param->_recved_data )
            {
                free(param->_recved_data);
                param->_recved_data = NULL;
                param->_recved_data_len = 0;
            }
            break;
        }
        case EMHC_NOTIFY_FINISHED:
        {
            break;
        }
        default:
            break;
    }
	return SUCCESS;
}

_int32 member_report_login_info(_u32 cmdid, MEMBER_PROTOCAL_IMPL *info, _int32 errcode, _u32 response_time)
{
	_int32 ret_val = 0;
	EM_HTTP_GET http_para = {0};
	_u32 http_id = 0;

	char url[MAX_URL_LEN] = {0};
	char ui_version[128] = {0};
	char host[MAX_HOST_NAME_LEN] = {0};
	_u32 server_ip = 0;
	_u32 bussiness_id = MEMBER_BUSSINESS_TYPE;
	_u32 platform = 0; /* 0客户端， 1web端*/
	em_settings_get_str_item("system.ui_version", ui_version);

	sd_strncpy(host, info->_main_server_host, info->_main_server_host_len);
	if( 0 != info->_server_index )
		server_ip = info->_servers[info->_server_index]._ip;
	
	sd_snprintf(url, MAX_URL_LEN -1, "http://%s:%u/report?cnt=1&cmdid0=%d&errorcode0=%d&responsetime0=%u&retrynum0=1&serverip0=%u&url0=&domain0=%s&b_type0=%u&platform0=%u&clientversion0=%s", 
		REPORT_SERVER_IP, REPORT_SERVER_PORT, cmdid, errcode, response_time, server_ip, host, bussiness_id, platform, ui_version);

	http_para._url = url;
	http_para._url_len = sd_strlen(http_para._url);
	http_para._user_data = &http_para;
	http_para._callback_fun = member_report_login_info_resp;
	ret_val = sd_malloc(16384, &http_para._recv_buffer);
	if( SUCCESS != ret_val )
		return ret_val;
	http_para._recv_buffer_size = 16384;
	http_para._timeout = 10;
	
	ret_val = em_http_get_impl(&http_para, &http_id, -1);

	if(ret_val != SUCCESS)
	{
		//deal with error
	}

	return ret_val;
}

#endif
// ------------------------------------------------------------------------

static _int32 member_encrypt_passwd(char * passwd, _u32 passwd_len, 
	char *md5_passwd, _u32 md5_size, _u32 *p_md5_len,
	char *rsa_passwd, _u32 rsa_size, _u32 *p_rsa_len)
{
	_int32 ret = SUCCESS;

	sd_assert(md5_size >= 33 && rsa_size >= 65);
	
	ret = sd_memset(md5_passwd, 0, md5_size);
	CHECK_VALUE(ret);

	ret = sd_memset(rsa_passwd, 0, rsa_size);
	CHECK_VALUE(ret);

	member_double_md5(md5_passwd, passwd, passwd_len);
	member_rsa(rsa_passwd, md5_passwd);
	*p_md5_len = 32;
	*p_rsa_len = 64;
	return SUCCESS;
}

static _int32 member_build_cmd_common_header(char **buf, _int32 *len, _int32 body_len)
{
	_int32 ret = SUCCESS;
	static _u32 s_seq = 1;

	ret = sd_set_int32_to_lt(buf, len, 100);
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(buf, len, s_seq++);
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(buf, len, body_len + 6);
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(buf, len, 0);
	CHECK_VALUE(ret);

	ret = sd_set_int16_to_lt(buf, len, 0);
	CHECK_VALUE(ret);

	return SUCCESS;
}

static _int32 member_parse_cmd_common_header(char **buf, _int32 *len, _int32 * p_body_len)
{
	_int32 ret = SUCCESS;
	_u32 version = 0;
	_u32 seq = 0;
	_u32 body_len = 0;
	_u32 platform_ver = 0;
	_u16 is_compressed = 0;

	ret = sd_get_int32_from_lt(buf, len, (_int32 *)&version);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(buf, len, (_int32 *)&seq);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(buf, len, (_int32 *)&body_len);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(buf, len, (_int32 *)&platform_ver);
	CHECK_VALUE(ret);

	ret = sd_get_int16_from_lt(buf, len, (_int16 *)&is_compressed);
	CHECK_VALUE(ret);

	*p_body_len = body_len;
	return ret;
}

static _int32 member_build_cmd_common_header_set_body_len(char *header, _int32 header_len, _int32 body_len)
{
	_int32 ret = SUCCESS;
	char *buf = header + 8;
	_int32 len = header_len - 8;
	ret = sd_set_int32_to_lt(&buf, &len, body_len + 6);
	return ret;
}

static _int32 member_build_cmd_login(MEMBER_PROTOCAL_LOGIN_INFO * p_member_login_info, 
	char *data, _u32 data_size, _u32 *p_data_len)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_size;
	char md5_passwd[33], rsa_passwd[65];
	_u32 md5_len = 0, rsa_len = 0;
	_u32 common_header_len = 0;
	char *thunder_version = (char*)member_get_version();
	_u32 thunder_version_len = sd_strlen(thunder_version);
	_int8 business_type = MEMBER_BUSSINESS_TYPE;

	ret = member_build_cmd_common_header(&buf, &len, 0);
	CHECK_VALUE(ret);

	common_header_len = data_size - len;
	/* CmdID */
	ret = sd_set_int16_to_lt(&buf, &len, 0x0001);
	CHECK_VALUE(ret);
	
	ret = sd_set_int32_to_lt(&buf, &len, p_member_login_info->_user_name_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_member_login_info->_user_name, p_member_login_info->_user_name_len);
	CHECK_VALUE(ret);

	ret = sd_set_int8(&buf, &len, (_int8)p_member_login_info->_login_type);
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, 0);	// session id 为空;
	CHECK_VALUE(ret);

	business_type = (_int8)p_member_login_info->_business_type;
	ret = sd_set_int8(&buf, &len, business_type);	// 无线business_type
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, thunder_version_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, thunder_version, thunder_version_len);
	CHECK_VALUE(ret);

	ret = member_encrypt_passwd(p_member_login_info->_passwd, p_member_login_info->_passwd_len,
		md5_passwd, 33, &md5_len, rsa_passwd, 65, &rsa_len);
	
	CHECK_VALUE(ret);
	ret = sd_set_int32_to_lt(&buf, &len, rsa_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, rsa_passwd, rsa_len);
	CHECK_VALUE(ret);
	
	ret = sd_set_int32_to_lt(&buf, &len, p_member_login_info->_peerid_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_member_login_info->_peerid, p_member_login_info->_peerid_len);
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, md5_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, md5_passwd, md5_len);
	CHECK_VALUE(ret);

	ret = member_build_cmd_common_header_set_body_len(data, data_size, (data_size - len - common_header_len));
	CHECK_VALUE(ret);

	*p_data_len = data_size - len;
	return SUCCESS;
}


_int32 member_build_cmd_sessionid_login(MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO* p_member_sessionid_login_info, 
	char *data, _u32 data_size, _u32 *p_data_len)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_size; /* remain len */
	char md5_passwd[33], rsa_passwd[65];
	_u32 md5_len = 0, rsa_len = 0;
	_u32 common_header_len = 0;
	char *thunder_version = (char*)member_get_version();
	_u32 thunder_version_len = sd_strlen(thunder_version);
	_int8 business_type = MEMBER_BUSSINESS_TYPE;

	ret = member_build_cmd_common_header(&buf, &len, 0);
	CHECK_VALUE(ret);

	common_header_len = data_size - len;
	/* CmdID */
	ret = sd_set_int16_to_lt(&buf, &len, CMD_ID_SESSIONID_LOGIN);
	CHECK_VALUE(ret);
	/* SessionID */
	ret = sd_set_int32_to_lt(&buf, &len, p_member_sessionid_login_info->_sessionid_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_member_sessionid_login_info->_sessionid, p_member_sessionid_login_info->_sessionid_len);
	CHECK_VALUE(ret);
	/* SessionType */
	ret = sd_set_int16_to_lt(&buf, &len, (_int16)p_member_sessionid_login_info->_sessionid_type);
	CHECK_VALUE(ret);
	/* BusinessType */
	ret = sd_set_int8(&buf, &len, (_int8)p_member_sessionid_login_info->_business_type); // 无线business_type
	CHECK_VALUE(ret);
	/* ClientVersion */
	ret = sd_set_int32_to_lt(&buf, &len, thunder_version_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, thunder_version, thunder_version_len);
	CHECK_VALUE(ret);
	/* PeerID */
	ret = sd_set_int32_to_lt(&buf, &len, p_member_sessionid_login_info->_peerid_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_member_sessionid_login_info->_peerid, p_member_sessionid_login_info->_peerid_len);
	CHECK_VALUE(ret);
	/* OriUserID */
	ret = sd_set_int64_to_lt(&buf, &len, p_member_sessionid_login_info->_origin_user_id);
	CHECK_VALUE(ret);
	/* OriBusinessType */
	ret = sd_set_int32_to_lt(&buf, &len, p_member_sessionid_login_info->_origin_business_type);
	CHECK_VALUE(ret);
	
	/* set common header length again */
	ret = member_build_cmd_common_header_set_body_len(data, data_size, (data_size - len - common_header_len));
	CHECK_VALUE(ret);

	*p_data_len = data_size - len;
	return SUCCESS;
}

_int32 member_parse_cmd_sessionid_login_resp(char *data, _u32 data_len, MEMBER_PROTOCAL_USER_INFO *p_user_info)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_len;
	_u8 error_code = 0;

	_int32 body_len = 0;
	_int16 cmd_id = 0;
	_int32 jump_key_len = 0;
	char * p_size = NULL;

	ret = member_parse_cmd_common_header(&buf, &len, &body_len);
	CHECK_VALUE(ret);
	/* CmdID */
	ret = sd_get_int16_from_lt(&buf, &len, &cmd_id);
	CHECK_VALUE(ret);
	/* ErrorCode */
	ret = sd_get_int8(&buf, &len, (_int8 *)&error_code);
	if (error_code != 0)
	{
		return error_code;
	}
	/* UserID */
	ret = sd_get_int64_from_lt(&buf, &len, (_int64 *)&p_user_info->_userno);
	CHECK_VALUE(ret);
	/* UserName */
	ret = sd_memset(p_user_info->_user_name, 0, MEMBER_PROTOCAL_USER_NAME_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_user_name_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_user_name, p_user_info->_user_name_len);
	CHECK_VALUE(ret);
	/* UserNewNo */
	ret = sd_get_int64_from_lt(&buf, &len, (_int64 *)&p_user_info->_user_new_no);
	CHECK_VALUE(ret);
	/* UserType */
	ret = sd_get_int8(&buf, &len, (_int8 *)&p_user_info->_user_type);
	CHECK_VALUE(ret);
	/* SessionID */
	ret = sd_memset(p_user_info->_session_id, 0, MEMBER_PROTOCAL_SESSION_ID_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_session_id_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_session_id, p_user_info->_session_id_len);
	CHECK_VALUE(ret);
	/* JumpKey */
	ret = sd_memset(p_user_info->_jumpkey, 0, MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_jumpkey_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_jumpkey, p_user_info->_jumpkey_len);
	CHECK_VALUE(ret);
	/* ImgURL */
	ret = sd_memset(p_user_info->_img_url, 0, MEMBER_PROTOCAL_IMG_URL_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_img_url_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_img_url, p_user_info->_img_url_len);
	CHECK_VALUE(ret);
	/* 改成100x100 */
	p_size = sd_strstr(p_user_info->_img_url,"/50x50",0);
	if(p_size!=NULL)
	{
		char img_url[MEMBER_PROTOCAL_IMG_URL_SIZE] = {0};
		p_size++;
		*p_size = '\0';
		p_size+=sd_strlen("50x50");
		sd_snprintf(img_url, MEMBER_PROTOCAL_IMG_URL_SIZE-1, "%s100x100%s",p_user_info->_img_url,p_size);
		sd_strncpy(p_user_info->_img_url, img_url, MEMBER_PROTOCAL_IMG_URL_SIZE-1);
		p_user_info->_img_url_len = sd_strlen(p_user_info->_img_url);
	}
	
	return SUCCESS;
}

static _int32 member_parse_cmd_login_resp(char *data, _u32 data_len, MEMBER_PROTOCAL_USER_INFO *p_user_info)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_len;
	_u8 error_code = 0;

	_int32 body_len = 0;
	_int16 cmd_id = 0;
	_int32 jump_key_len = 0;
	char * p_size = NULL;

	ret = member_parse_cmd_common_header(&buf, &len, &body_len);
	CHECK_VALUE(ret);

	ret = sd_get_int16_from_lt(&buf, &len, &cmd_id);
	CHECK_VALUE(ret);
	
	ret = sd_get_int8(&buf, &len, (_int8 *)&error_code);
	CHECK_VALUE(ret);

	if (error_code != 0)
	{
		return error_code;
	}
	
	ret = sd_get_int64_from_lt(&buf, &len, (_int64 *)&p_user_info->_userno);
	CHECK_VALUE(ret);

	ret = sd_memset(p_user_info->_user_name, 0, MEMBER_PROTOCAL_USER_NAME_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_user_name_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_user_name, p_user_info->_user_name_len);
	CHECK_VALUE(ret);

	ret = sd_get_int64_from_lt(&buf, &len, (_int64 *)&p_user_info->_user_new_no);
	CHECK_VALUE(ret);

	ret = sd_get_int8(&buf, &len, (_int8 *)&p_user_info->_user_type);
	CHECK_VALUE(ret);

	ret = sd_memset(p_user_info->_session_id, 0, MEMBER_PROTOCAL_SESSION_ID_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_session_id_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_session_id, p_user_info->_session_id_len);
	CHECK_VALUE(ret);

	// 不需要jump_key_len，跳过。
	ret = sd_get_int32_from_lt(&buf, &len, &jump_key_len);
	CHECK_VALUE(ret);
	sd_memcpy(p_user_info->_jumpkey, buf, jump_key_len);
	p_user_info->_jumpkey_len = jump_key_len;
	if (jump_key_len > len)
	{
		CHECK_VALUE(BUFFER_OVERFLOW);
	}
	buf += jump_key_len;
	len -= jump_key_len;

	ret = sd_memset(p_user_info->_img_url, 0, MEMBER_PROTOCAL_IMG_URL_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_img_url_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_img_url, p_user_info->_img_url_len);
	CHECK_VALUE(ret);
	
	/* 改成100x100 */
	p_size = sd_strstr(p_user_info->_img_url,"/50x50",0);
	if(p_size!=NULL)
	{
		char img_url[MEMBER_PROTOCAL_IMG_URL_SIZE] = {0};
		p_size++;
		*p_size = '\0';
		p_size+=sd_strlen("50x50");
		sd_snprintf(img_url, MEMBER_PROTOCAL_IMG_URL_SIZE-1, "%s100x100%s",p_user_info->_img_url,p_size);
		sd_strncpy(p_user_info->_img_url, img_url, MEMBER_PROTOCAL_IMG_URL_SIZE-1);
		p_user_info->_img_url_len = sd_strlen(p_user_info->_img_url);
	}
	
	return SUCCESS;
}

static _int32 member_build_cmd_query_user_info(MEMBER_PROTOCAL_USER_INFO * p_member_user_info, 
	char *data, _u32 data_size, _u32 *p_data_len)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_size;
	_u32 common_header_len = 0;
	char *thunder_version = (char*)member_get_version();
	_u32 thunder_version_len = sd_strlen(thunder_version);;

	ret = member_build_cmd_common_header(&buf, &len, 0);
	CHECK_VALUE(ret);

	common_header_len = data_size - len;

	ret = sd_set_int16_to_lt(&buf, &len, 0x0003);
	CHECK_VALUE(ret);
	
	ret = sd_set_int64_to_lt(&buf, &len, p_member_user_info->_userno);
	CHECK_VALUE(ret);
	
	ret = sd_set_int32_to_lt(&buf, &len, p_member_user_info->_session_id_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_member_user_info->_session_id, p_member_user_info->_session_id_len);
	CHECK_VALUE(ret);

	ret = sd_set_int8(&buf, &len, MEMBER_BUSSINESS_TYPE);	// 无线business_type
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, thunder_version_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, thunder_version, thunder_version_len);
	CHECK_VALUE(ret);

	ret = member_build_cmd_common_header_set_body_len(data, data_size, (data_size - len - common_header_len));
	CHECK_VALUE(ret);

	*p_data_len = data_size - len;
	return SUCCESS;
}

static _int32 member_parse_cmd_query_user_info_resp(char *data, _u32 data_len, MEMBER_PROTOCAL_USER_INFO *p_user_info)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_len;
	_u8 error_code = 0;

	_int32 body_len = 0;
	_int16 cmd_id = 0;

	ret = member_parse_cmd_common_header(&buf, &len, &body_len);
	CHECK_VALUE(ret);

	ret = sd_get_int16_from_lt(&buf, &len, &cmd_id);
	CHECK_VALUE(ret);
	
	ret = sd_get_int8(&buf, &len, (_int8 *)&error_code);
	CHECK_VALUE(ret);

	if (error_code != 0)
	{
		return error_code;
	}
	
	ret = sd_memset(p_user_info->_nickname, 0, MEMBER_PROTOCAL_USER_NAME_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_nickname_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_nickname, p_user_info->_nickname_len);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_account);
	CHECK_VALUE(ret);
	
	ret = sd_get_int8(&buf, &len, (_int8 *)&p_user_info->_rank);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_order);
	CHECK_VALUE(ret);

	ret = sd_get_int8(&buf, &len, (_int8 *)&p_user_info->_is_name2newno);
	CHECK_VALUE(ret);

	ret = sd_memset(p_user_info->_country, 0, MEMBER_PROTOCAL_ADDR_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_country_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_country, p_user_info->_country_len);
	CHECK_VALUE(ret);

	ret = sd_memset(p_user_info->_province, 0, MEMBER_PROTOCAL_ADDR_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_province_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_province, p_user_info->_province_len);
	CHECK_VALUE(ret);

	ret = sd_memset(p_user_info->_city, 0, MEMBER_PROTOCAL_ADDR_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_city_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_city, p_user_info->_city_len);
	CHECK_VALUE(ret);

	ret = sd_get_int8(&buf, &len, (_int8 *)&p_user_info->_is_special_num);
	CHECK_VALUE(ret);

	ret = sd_get_int8(&buf, &len, (_int8 *)&p_user_info->_role);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_today_score);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_allow_score);
	CHECK_VALUE(ret);

	ret = sd_memset(p_user_info->_personal_sign, 0, MEMBER_PROTOCAL_PERSIONAL_SIGN_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_personal_sign_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_personal_sign, p_user_info->_personal_sign_len);
	CHECK_VALUE(ret);

	return SUCCESS;
}

static _int32 member_build_cmd_query_portal(char *data, _u32 data_size, _u32 *p_data_len,
	MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_size;
	_u32 common_header_len = 0;
	char *thunder_version = (char*)member_get_version();
	_u32 thunder_version_len = sd_strlen(thunder_version);
	char tmp_ip[32];
	_u32 tmp_ip_len = 0;

	ret = member_build_cmd_common_header(&buf, &len, 0);
	CHECK_VALUE(ret);

	common_header_len = data_size - len;

	ret = sd_set_int16_to_lt(&buf, &len, 0x0017);
	CHECK_VALUE(ret);
	
	ret = sd_set_int8(&buf, &len, MEMBER_BUSSINESS_TYPE);	// 无线business_type
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, thunder_version_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, thunder_version, thunder_version_len);
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, p_member->_main_server_host_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_member->_main_server_host, p_member->_main_server_host_len);
	CHECK_VALUE(ret);

	ret = sd_inet_ntoa(p_member->_servers[0]._ip, tmp_ip, 32);
	tmp_ip_len = sd_strlen(tmp_ip);
	ret = sd_set_int32_to_lt(&buf, &len, tmp_ip_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, tmp_ip, tmp_ip_len);
	CHECK_VALUE(ret);

	ret = sd_set_int16_to_lt(&buf, &len, p_member->_servers[0]._port);
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, 3);	//  3：帐号登录; 4：VIP信息查询
	CHECK_VALUE(ret);

	ret = member_build_cmd_common_header_set_body_len(data, data_size, (data_size - len - common_header_len));
	CHECK_VALUE(ret);

	*p_data_len = data_size - len;
	return SUCCESS;
}

static _int32 member_parse_cmd_query_portal_resp(char *data, _u32 data_len,
	MEMBER_SERVER_INFO bak_servers[], _u32 size, _u32 *p_bak_server_count)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_len;
	//_u8 error_code = 0;

	_int32 body_len = 0;
	_int16 cmd_id = 0;
	_u32 i = 0;
	char tmp_ip[32];
	_u32 tmp_ip_len = 0;

	ret = member_parse_cmd_common_header(&buf, &len, &body_len);
	CHECK_VALUE(ret);

	ret = sd_get_int16_from_lt(&buf, &len, &cmd_id);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)p_bak_server_count);
	CHECK_VALUE(ret);

	if (*p_bak_server_count > size)
	{
		*p_bak_server_count = size;
	}

	for (i = 0; i < (*p_bak_server_count); ++i)
	{
		ret = sd_memset(tmp_ip, 0, 32);
		CHECK_VALUE(ret);
		ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&tmp_ip_len);
		CHECK_VALUE(ret);
		ret = sd_get_bytes(&buf, &len, tmp_ip, tmp_ip_len);
		CHECK_VALUE(ret);

		tmp_ip[tmp_ip_len] = 0;
		ret = sd_inet_aton(tmp_ip, &bak_servers[i]._ip);
		CHECK_VALUE(ret);
		
		ret = sd_get_int16_from_lt(&buf, &len, (_int16 *)&bak_servers[i]._port);
		CHECK_VALUE(ret);
		LOG_DEBUG("member_parse_cmd_query_portal_resp:%s:%u!",tmp_ip,bak_servers[i]._port);
	}

	return SUCCESS;
}

static _int32 member_build_cmd_login_vip(MEMBER_PROTOCAL_USER_INFO * p_member_user_info, 
	char *data, _u32 data_size, _u32 *p_data_len)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_size;
	_u32 common_header_len = 0;//, body_len = 0;
	char *thunder_version = (char*)member_get_version();
	_u32 thunder_version_len = sd_strlen(thunder_version);;

	ret = member_build_cmd_common_header(&buf, &len, 0);
	CHECK_VALUE(ret);

	common_header_len = data_size - len;

	ret = sd_set_int16_to_lt(&buf, &len, 0x0013);
	CHECK_VALUE(ret);
	
	ret = sd_set_int64_to_lt(&buf, &len, p_member_user_info->_userno);
	CHECK_VALUE(ret);
	
	ret = sd_set_int8(&buf, &len, MEMBER_BUSSINESS_TYPE);	// 无线business_type
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, thunder_version_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, thunder_version, thunder_version_len);
	CHECK_VALUE(ret);

	ret = member_build_cmd_common_header_set_body_len(data, data_size, (data_size - len - common_header_len));
	CHECK_VALUE(ret);

	*p_data_len = data_size - len;
	return SUCCESS;
}

static _int32 member_parse_cmd_login_vip_resp(char *data, _u32 data_len, MEMBER_PROTOCAL_USER_INFO *p_user_info)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_len;
	_u16 error_code = 0;	// 受不了，error_code 的大小不统一。

	_int32 body_len = 0;
	_int16 cmd_id = 0;

	ret = member_parse_cmd_common_header(&buf, &len, &body_len);
	CHECK_VALUE(ret);

	ret = sd_get_int16_from_lt(&buf, &len, &cmd_id);
	CHECK_VALUE(ret);
	
	ret = sd_get_int16_from_lt(&buf, &len, (_int16 *)&error_code);
	CHECK_VALUE(ret);

	if (error_code != 0)
	{
		return error_code;
	}

	ret = sd_get_int8(&buf, &len, (_int8 *)&p_user_info->_is_vip);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_vas);
	CHECK_VALUE(ret);

	ret = sd_get_int16_from_lt(&buf, &len, (_int16 *)&p_user_info->_level);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_grow);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_payid);
	CHECK_VALUE(ret);

	ret = sd_memset(p_user_info->_payid_name, 0, MEMBER_PROTOCAL_PAYID_NAME_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_payid_name_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_payid_name, p_user_info->_payid_name_len);
	CHECK_VALUE(ret);

	ret = sd_memset(p_user_info->_expire_date, 0, MEMBER_PROTOCAL_DATE_SIZE);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_expire_date_len);
	CHECK_VALUE(ret);
	ret = sd_get_bytes(&buf, &len, p_user_info->_expire_date, p_user_info->_expire_date_len);
	CHECK_VALUE(ret);

	ret = sd_get_int8(&buf, &len, (_int8 *)&p_user_info->_is_auto_deduct);
	CHECK_VALUE(ret);

	ret = sd_get_int8(&buf, &len, (_int8 *)&p_user_info->_is_remind);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_user_info->_daily);
	CHECK_VALUE(ret);

	return SUCCESS;
}

// value_buf 需保证有足够的空间存放
static _int32 member_get_value_from_string(const char* buffer, _u32 buffer_len, const char* search_str, char* value_buf)
{
	char* p1 = NULL, *p2 = NULL;
	_u32 len = 0;
	if( NULL != (p1 = sd_strstr(buffer, search_str, 0)) )
	{
		p1 += sd_strlen(search_str)+1;
		len = buffer_len - (p1 - buffer);
		if((p2 = sd_strchr(p1, '&', 0)) && (p2 - p1 < len))
		{
			sd_strncpy(value_buf, p1, p2 -p1);
			value_buf[p2 -p1] = '\0';
			return SUCCESS;
		}
		else
		{
			sd_strncpy(value_buf, p1, len);
			value_buf[len] = '\0';
			return SUCCESS;
		}
	}
	return -1;
}

static _int32 member_parse_http_query_vip_resp(char *data, _u32 data_len, MEMBER_PROTOCAL_USER_INFO *p_user_info)
{
	_int32 ret = 0;
	char tmp[64] = {0};
	_int32 vas_type = 0;

	if(SUCCESS == member_get_value_from_string(data, data_len, "ret", tmp))
	{
		ret = sd_atoi(tmp);
		if(ret != SUCCESS)
		{
			return ret;
		}
	}
	if(SUCCESS == member_get_value_from_string(data, data_len, "isvip", tmp))
	{
		p_user_info->_is_vip = (_u8)sd_atoi(tmp);
	}
	if(SUCCESS == member_get_value_from_string(data, data_len, "isyear", tmp))
	{
		p_user_info->_is_year = (_u8)sd_atoi(tmp);
	}
	if(SUCCESS == member_get_value_from_string(data, data_len, "uservas", tmp))
	{
		p_user_info->_vas = sd_atoi(tmp);
	}
	if(SUCCESS == member_get_value_from_string(data, data_len, "level", tmp))
	{
		p_user_info->_level = (_u16)sd_atoi(tmp);
	}
	if(SUCCESS == member_get_value_from_string(data, data_len, "grow", tmp))
	{
		p_user_info->_grow = sd_atoi(tmp);
	}	
	if(SUCCESS == member_get_value_from_string(data, data_len, "payid", tmp))
	{
		p_user_info->_payid = sd_atoi(tmp);
	}
	if(SUCCESS == member_get_value_from_string(data, data_len, "payname", tmp))
	{
		p_user_info->_payid_name_len = sd_strlen(tmp);
		sd_strncpy(p_user_info->_payid_name, tmp, p_user_info->_payid_name_len);
	}
	if(SUCCESS == member_get_value_from_string(data, data_len, "daily", tmp))
	{
		p_user_info->_daily= sd_atoi(tmp);
	}	
	if( SUCCESS == member_get_value_from_string(data, data_len, "expire", tmp) )
	{
		p_user_info->_expire_date_len = sd_strlen(tmp);
		sd_strncpy(p_user_info->_expire_date, tmp, p_user_info->_expire_date_len);
	}
	if( SUCCESS == member_get_value_from_string(data, data_len, "month_expire", tmp) )
	{
		// 针对手机月付会员(手机包月是会自动续费) 没有过期时间的情况，使用month_expire字段(为了自动登录时的过期时间判断)
		if( 0 == p_user_info->_expire_date_len || p_user_info->_expire_date_len < 5 )
		{
			p_user_info->_expire_date_len = sd_strlen(tmp);
			sd_strncpy(p_user_info->_expire_date, tmp, p_user_info->_expire_date_len);
		}
	}
	if(SUCCESS == member_get_value_from_string(data, data_len, "autodeduct", tmp))
	{
		p_user_info->_is_auto_deduct = (_u8)sd_atoi(tmp);
	}	
	if(SUCCESS == member_get_value_from_string(data, data_len, "remind", tmp))
	{
		p_user_info->_is_remind = (_u8)sd_atoi(tmp);
	}	
	if(SUCCESS == member_get_value_from_string(data, data_len, "vas_type", tmp))
	{
		vas_type = sd_atoi(tmp);
		if(p_user_info->_is_vip == 1)
		{
			if(vas_type == 3)// 3 为白金会员
				p_user_info->_is_platinum = 1;
			else
				p_user_info->_is_platinum = 0;
		}
		p_user_info->_vas_type = vas_type;
	}

	return SUCCESS;
	
}

static _int32 member_build_cmd_query_portal_vip(char *data, _u32 data_size, _u32 *p_data_len,
	MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_size;
	_u32 common_header_len = 0;//, body_len = 0;
	char *thunder_version = (char*)member_get_version();
	_u32 thunder_version_len = sd_strlen(thunder_version);
	char tmp_ip[32];
	_u32 tmp_ip_len = 0;

	ret = member_build_cmd_common_header(&buf, &len, 0);
	CHECK_VALUE(ret);

	common_header_len = data_size - len;

	ret = sd_set_int16_to_lt(&buf, &len, 0x0017);
	CHECK_VALUE(ret);
	
	ret = sd_set_int8(&buf, &len, MEMBER_BUSSINESS_TYPE);	// 无线business_type
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, thunder_version_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, thunder_version, thunder_version_len);
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, p_member->_vip_main_server_host_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_member->_vip_main_server_host, p_member->_vip_main_server_host_len);
	CHECK_VALUE(ret);

	ret = sd_inet_ntoa(p_member->_vip_servers[0]._ip, tmp_ip, 32);
	tmp_ip_len = sd_strlen(tmp_ip);
	ret = sd_set_int32_to_lt(&buf, &len, tmp_ip_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, tmp_ip, tmp_ip_len);
	CHECK_VALUE(ret);

	ret = sd_set_int16_to_lt(&buf, &len, p_member->_vip_servers[0]._port);
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, 4);		//  3：帐号登录; 4：VIP信息查询
	CHECK_VALUE(ret);

	ret = member_build_cmd_common_header_set_body_len(data, data_size, (data_size - len - common_header_len));
	CHECK_VALUE(ret);

	*p_data_len = data_size - len;
	return SUCCESS;
}

static _int32 member_parse_cmd_query_portal_vip_resp(char *data, _u32 data_len,
	MEMBER_SERVER_INFO bak_servers[], _u32 size, _u32 *p_bak_server_count)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_len;
	//_u8 error_code = 0;

	_int32 body_len = 0;
	_int16 cmd_id = 0;
	_u32 i = 0;
	char tmp_ip[32];
	_u32 tmp_ip_len = 0;

	ret = member_parse_cmd_common_header(&buf, &len, &body_len);
	CHECK_VALUE(ret);

	ret = sd_get_int16_from_lt(&buf, &len, &cmd_id);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)p_bak_server_count);
	CHECK_VALUE(ret);

	if (*p_bak_server_count > size)
	{
		*p_bak_server_count = size;
	}

	for (i = 0; i < (*p_bak_server_count); ++i)
	{
		ret = sd_memset(tmp_ip, 0, 32);
		CHECK_VALUE(ret);
		ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&tmp_ip_len);
		CHECK_VALUE(ret);
		ret = sd_get_bytes(&buf, &len, tmp_ip, tmp_ip_len);
		CHECK_VALUE(ret);

		tmp_ip[tmp_ip_len] = 0;
		ret = sd_inet_aton(tmp_ip, &bak_servers[i]._ip);
		CHECK_VALUE(ret);
		
		ret = sd_get_int16_from_lt(&buf, &len, (_int16 *)&bak_servers[i]._port);
		CHECK_VALUE(ret);
		LOG_DEBUG("member_parse_cmd_query_portal_vip_resp:%s:%u!",tmp_ip,bak_servers[i]._port);
	}

	return SUCCESS;
}

static _int32 member_build_cmd_ping(MEMBER_PROTOCAL_PING *p_ping, char *data, 
	_u32 data_size, _u32 *p_data_len)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_size;
	_u32 common_header_len = 0;//, body_len = 0;
	char *thunder_version = (char*)member_get_version();
	_u32 thunder_version_len = sd_strlen(thunder_version);;

	ret = member_build_cmd_common_header(&buf, &len, 0);
	CHECK_VALUE(ret);

	common_header_len = data_size - len;

	ret = sd_set_int16_to_lt(&buf, &len, 0x0011);
	CHECK_VALUE(ret);
	
	ret = sd_set_int64_to_lt(&buf, &len, p_ping->_userno);
	CHECK_VALUE(ret);
	
	ret = sd_set_int32_to_lt(&buf, &len, p_ping->_session_id_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_ping->_session_id, p_ping->_session_id_len);
	CHECK_VALUE(ret);

	ret = sd_set_int8(&buf, &len, MEMBER_BUSSINESS_TYPE);	// 无线business_type
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, thunder_version_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, thunder_version, thunder_version_len);
	CHECK_VALUE(ret);

	ret = member_build_cmd_common_header_set_body_len(data, data_size, (data_size - len - common_header_len));
	CHECK_VALUE(ret);

	*p_data_len = data_size - len;
	return SUCCESS;
}

static _int32 member_parse_cmd_ping_resp(char *data, _u32 data_len, 
 	MEMBER_PROTOCAL_PING_RESP * p_ping_resp)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_len;
	_u8 error_code = 0;

	_int32 body_len = 0;
	_int16 cmd_id = 0;
	_u8 bussiness_type = 0;
	char thunder_version[32];
	_u32 thunder_version_len = 0;

	ret = member_parse_cmd_common_header(&buf, &len, &body_len);
	CHECK_VALUE(ret);

	ret = sd_get_int16_from_lt(&buf, &len, &cmd_id);
	CHECK_VALUE(ret);
	
	ret = sd_get_int8(&buf, &len, (_int8 *)&error_code);
	CHECK_VALUE(ret);

	if (error_code != 0)
	{
		return error_code;
	}
	
	ret = sd_get_int64_from_lt(&buf, &len, (_int64 *)&p_ping_resp->_userno);
	CHECK_VALUE(ret);

	ret = sd_get_int8(&buf, &len, (_int8 *)&bussiness_type);
	CHECK_VALUE(ret);

	ret = sd_memset(thunder_version, 0, 32);
	CHECK_VALUE(ret);
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&thunder_version_len);
	CHECK_VALUE(ret);
	thunder_version_len = (thunder_version_len <= 32)? thunder_version_len: 32;
	ret = sd_get_bytes(&buf, &len, thunder_version, thunder_version_len);
	CHECK_VALUE(ret);

	ret = sd_get_int8(&buf, &len, (_int8 *)&p_ping_resp->_should_kick);
	CHECK_VALUE(ret);

	return SUCCESS;
}

static _int32 member_build_cmd_report_download_file(MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE *p_rpt_dl_file,
	char *data, _u32 data_size, _u32 *p_data_len)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_size;
	_u32 common_header_len = 0;//, body_len = 0;
	char *thunder_version = (char*)member_get_version();
	_u32 thunder_version_len = sd_strlen(thunder_version);;

	ret = member_build_cmd_common_header(&buf, &len, 0);
	CHECK_VALUE(ret);

	common_header_len = data_size - len;

	ret = sd_set_int16_to_lt(&buf, &len, 0x0005);
	CHECK_VALUE(ret);
	
	ret = sd_set_int64_to_lt(&buf, &len, p_rpt_dl_file->_userno);
	CHECK_VALUE(ret);
	
	ret = sd_set_int32_to_lt(&buf, &len, p_rpt_dl_file->_session_id_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_rpt_dl_file->_session_id, p_rpt_dl_file->_session_id_len);
	CHECK_VALUE(ret);

	ret = sd_set_int8(&buf, &len, MEMBER_BUSSINESS_TYPE);	// 无线business_type
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, thunder_version_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, thunder_version, thunder_version_len);
	CHECK_VALUE(ret);

	ret = sd_set_int64_to_lt(&buf, &len, p_rpt_dl_file->_dl_file_size);
	CHECK_VALUE(ret);
	
	ret = sd_set_int32_to_lt(&buf, &len, p_rpt_dl_file->_cid_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_rpt_dl_file->_cid, p_rpt_dl_file->_cid_len);
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, p_rpt_dl_file->_url_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_rpt_dl_file->_url, p_rpt_dl_file->_url_len);
	CHECK_VALUE(ret);

	ret = sd_set_int8(&buf, &len, p_rpt_dl_file->_dead_link);
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, p_rpt_dl_file->_saved_time);
	CHECK_VALUE(ret);

	ret = member_build_cmd_common_header_set_body_len(data, data_size, (data_size - len - common_header_len));
	CHECK_VALUE(ret);

	*p_data_len = data_size - len;
	return SUCCESS;
}

static _int32 member_parse_cmd_report_download_file_resp(char *data, _u32 data_len, 
 	MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE_RESP * p_rpt_dl_file_resp)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_len;
	_u8 error_code = 0;

	_int32 body_len = 0;
	_int16 cmd_id = 0;
	//_u8 bussiness_type = 0;
	//_u32 thunder_version_len = 0;

	ret = member_parse_cmd_common_header(&buf, &len, &body_len);
	CHECK_VALUE(ret);

	ret = sd_get_int16_from_lt(&buf, &len, &cmd_id);
	CHECK_VALUE(ret);
	
	ret = sd_get_int8(&buf, &len, (_int8 *)&error_code);
	CHECK_VALUE(ret);

	if (error_code != 0)
	{
		return error_code;
	}
	
	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_rpt_dl_file_resp->_today_score);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_rpt_dl_file_resp->_df_score);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_rpt_dl_file_resp->_total_score);
	CHECK_VALUE(ret);

	ret = sd_get_int32_from_lt(&buf, &len, (_int32 *)&p_rpt_dl_file_resp->_allow_score);
	CHECK_VALUE(ret);

	return SUCCESS;
}

static _int32 member_build_cmd_logout(MEMBER_PROTOCAL_LOGOUT_INFO *p_logout, char *data, 
	_u32 data_size, _u32 *p_data_len)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_size;
	_u32 common_header_len = 0;//, body_len = 0;
	char *thunder_version = (char*)member_get_version();
	_u32 thunder_version_len = sd_strlen(thunder_version);;

	ret = member_build_cmd_common_header(&buf, &len, 0);
	CHECK_VALUE(ret);

	common_header_len = data_size - len;

	ret = sd_set_int16_to_lt(&buf, &len, 0x0009);
	CHECK_VALUE(ret);
	
	ret = sd_set_int64_to_lt(&buf, &len, p_logout->_userno);
	CHECK_VALUE(ret);
	
	ret = sd_set_int32_to_lt(&buf, &len, p_logout->_session_id_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, p_logout->_session_id, p_logout->_session_id_len);
	CHECK_VALUE(ret);

	ret = sd_set_int8(&buf, &len, MEMBER_BUSSINESS_TYPE);	// 无线businesstype
	CHECK_VALUE(ret);

	ret = sd_set_int32_to_lt(&buf, &len, thunder_version_len);
	CHECK_VALUE(ret);
	ret = sd_set_bytes(&buf, &len, thunder_version, thunder_version_len);
	CHECK_VALUE(ret);

	ret = member_build_cmd_common_header_set_body_len(data, data_size, (data_size - len - common_header_len));
	CHECK_VALUE(ret);

	*p_data_len = data_size - len;
	return SUCCESS;
}

static _int32 member_parse_cmd_logout_resp(char *data, _u32 data_len)
{
	_int32 ret = SUCCESS;
	char *buf = data;
	_int32 len = (_int32)data_len;
	_u8 error_code = 0;

	_int32 body_len = 0;
	_int16 cmd_id = 0;

	ret = member_parse_cmd_common_header(&buf, &len, &body_len);
	CHECK_VALUE(ret);

	ret = sd_get_int16_from_lt(&buf, &len, &cmd_id);
	CHECK_VALUE(ret);
	
	ret = sd_get_int8(&buf, &len, (_int8 *)&error_code);
	CHECK_VALUE(ret);

	if (error_code != 0)
	{
		return error_code;
	}
	
	return SUCCESS;
}

static _int32 member_insert_section_to_register_query(char **data, _u32 *data_size,
	char *section, _u32 section_len, char *value, _u32 value_len, BOOL is_utf8)
{
	char utf8_str[128] = {0};
	_int32 ret = SUCCESS; 
	_u32 utf8_str_len = 128;

	if(value_len==0)
		return SUCCESS;
	
	if(*data_size < value_len + section_len + 1)
		return BUFFER_OVERFLOW;

	sd_memcpy(*data, section, section_len);
	*data_size -= section_len;
	*data += section_len;

	sd_memcpy(*data, "=", 1);
	*data_size -= 1;
	*data += 1;

	if (is_utf8)
	{
		sd_memcpy(*data, value, value_len);
		*data_size -= value_len;
		*data += value_len;
	}
	else
	{
		ret = sd_gbk_2_utf8(value, value_len, utf8_str, &utf8_str_len);
		CHECK_VALUE(ret);

		sd_memcpy(*data, utf8_str, utf8_str_len);
		*data_size -= utf8_str_len;
		*data += utf8_str_len;
	}
	
	return SUCCESS;
}

static _int32 member_insert_and_char_to_register_query(char **data, _u32 *data_size)
{
	if (*data_size < 1)
		return BUFFER_OVERFLOW;

	sd_memcpy(*data, "&", 1);
	*data_size -= 1;
	*data += 1;
	
	return SUCCESS;
}

static _int32 member_build_cmd_register_check_id_query(MEMBER_PROTOCAL_REGISTER_CHECK_ID *check_id, char *data, 
	_u32 data_size, _u32 *p_data_len)
{
	_int32 ret = SUCCESS;
	char tmp[16] = {0}, build_buf[1024] = {0}, *buf = build_buf;
	_u32 len = 1024;

	ret = member_insert_section_to_register_query(&buf, &len, "request", sd_strlen("request"), "name2id", sd_strlen("name2id"), FALSE);
	CHECK_VALUE(ret);

	ret = member_insert_and_char_to_register_query(&buf, &len);
	CHECK_VALUE(ret);
	ret = member_insert_section_to_register_query(&buf, &len, "userid", sd_strlen("userid"), check_id->_userid, sd_strlen(check_id->_userid),FALSE);
	CHECK_VALUE(ret);

	ret = member_insert_and_char_to_register_query(&buf, &len);
	CHECK_VALUE(ret);
	sd_memset(tmp, 0, 16);
	sd_i32toa(check_id->_usertype, tmp, 16, 10);
	ret = member_insert_section_to_register_query(&buf, &len, "usertype", sd_strlen("usertype"), tmp, sd_strlen(tmp), FALSE);
	CHECK_VALUE(ret);

	sd_memset(data, 0, data_size);
	sd_strncpy(data, "c=", 2);
	sd_base64_encode((const _u8*)build_buf, 1024 - len, data + 2);

	*p_data_len = sd_strlen(data);
	return SUCCESS;
}


static _int32 member_build_cmd_register_query(MEMBER_PROTOCAL_REGISTER_INFO *register_info, char *data, 
	_u32 data_size, _u32 *p_data_len, BOOL is_utf8)
{
	_int32 ret = SUCCESS;
	char tmp[16] = {0}, build_buf[1024] = {0}, *buf = build_buf;
	_u32 len = 1024;

	ret = member_insert_section_to_register_query(&buf, &len, "request", sd_strlen("request"), "registeruser", sd_strlen("registeruser"), is_utf8);
	CHECK_VALUE(ret);

	ret = member_insert_and_char_to_register_query(&buf, &len);
	CHECK_VALUE(ret);
	ret = member_insert_section_to_register_query(&buf, &len, "userid", sd_strlen("userid"), register_info->_userid, sd_strlen(register_info->_userid), is_utf8);
	CHECK_VALUE(ret);

	ret = member_insert_and_char_to_register_query(&buf, &len);
	CHECK_VALUE(ret);
	sd_memset(tmp, 0, 16);
	sd_i32toa(register_info->_usertype, tmp, 16, 10);
	ret = member_insert_section_to_register_query(&buf, &len, "usertype", sd_strlen("usertype"), tmp, sd_strlen(tmp), is_utf8);
	CHECK_VALUE(ret);

	ret = member_insert_and_char_to_register_query(&buf, &len);
	CHECK_VALUE(ret);
	ret = member_insert_section_to_register_query(&buf, &len, "md5psw", sd_strlen("md5psw"), register_info->_password, sd_strlen(register_info->_password), is_utf8);
	CHECK_VALUE(ret);

	ret = member_insert_and_char_to_register_query(&buf, &len);
	CHECK_VALUE(ret);
	sd_memset(tmp, 0, 16);
	sd_i32toa(register_info->_registertype, tmp, 16, 10);
	ret = member_insert_section_to_register_query(&buf, &len, "registertype", sd_strlen("registertype"), tmp, sd_strlen(tmp), is_utf8);
	CHECK_VALUE(ret);

	ret = member_insert_and_char_to_register_query(&buf, &len);
	CHECK_VALUE(ret);
	ret = member_insert_section_to_register_query(&buf, &len, "nickname", sd_strlen("nickname"), register_info->_nickname, sd_strlen(register_info->_nickname), is_utf8);
	CHECK_VALUE(ret);

	sd_memset(data, 0, data_size);
	sd_strncpy(data, "c=", 2);
	sd_base64_encode((const _u8*)build_buf, 1024 - len, data + 2);

	ret = sd_replace_str(data, "+", "%2B");
	CHECK_VALUE(ret);
	ret = sd_replace_str(data, "&", "%26");
	CHECK_VALUE(ret);

	*p_data_len = sd_strlen(data);

	return SUCCESS;
}

static _int32 member_protocal_register_cmd_return_xml_node_proc(void *user_data, void *parent_node, _int32 parent_type, _int32 depth, char const *node_name, void **node, _int32 *node_type)
{
	 if (!sd_stricmp(node_name, "Command"))
	{
		*node = (void*)XML_NODE_TYPE_COMMAND;
		*node_type = XML_NODE_TYPE_COMMAND;
	}
	
	return SUCCESS;
}

static _int32 member_protocal_register_cmd_return_xml_node_end_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *node_name)
{
	return SUCCESS;
}

static _int32 member_protocal_register_cmd_return_xml_atrr_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *attr_name, char const *attr_value)
{
	if (node == (void*)XML_NODE_TYPE_COMMAND && node_type == XML_NODE_TYPE_COMMAND)
	{
		if (!sd_stricmp(attr_name, "id"))
		{
			sd_strncpy(((MEMBER_PROTOCAL_REGISTER_RESULT_INFO*)user_data)->_command_id, attr_value, sd_strlen(attr_value));
		}

		else if (!sd_stricmp(attr_name, "result"))
		{
			((MEMBER_PROTOCAL_REGISTER_RESULT_INFO*)user_data)->_result = sd_atoi(attr_value);
		}
	}
	
	return SUCCESS;
}

static _int32 member_protocal_register_cmd_return_xml_value_proc(void *user_data, void *node, _int32 node_type, _int32 depth, char const *value)
{
	return SUCCESS;
}

static _int32 member_parse_cmd_register_resp(char *data, _u32 data_len, MEMBER_PROTOCAL_REGISTER_RESULT_INFO *result_info)
{
#ifdef ENABLE_XML
	XML_SERVICE xml_service = NULL;
	_int32 ret = SUCCESS;

	xml_service = create_xml_service();

	ret = read_xml_buffer(xml_service, (char*)data, data_len, result_info,
		member_protocal_register_cmd_return_xml_node_proc, member_protocal_register_cmd_return_xml_node_end_proc,
		member_protocal_register_cmd_return_xml_atrr_proc, member_protocal_register_cmd_return_xml_value_proc);

	destroy_xml_service(xml_service);
	return ret;
#else
	sd_assert(FALSE);
	return NOT_IMPLEMENT;
#endif /* ENABLE_XML  */	
}


// ------------------------------------------------------------------------

typedef _int32 (*MEMBER_SEND_CMD_RESP)(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len);

typedef struct tag_MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA
{
	MEMBER_SEND_CMD_RESP _member_send_cmd_resp;
	MEMBER_PROTOCAL_IMPL * _p_member;
	char                 *_recv_buffer;
	_u32                  _recv_len;
}MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA;

typedef struct tag_MEMBER_PROTOCAL_SEND_CALLBACK_ALL_ARG
{
	void * _user_data;
	_int32 _result;
	void * _recv_buffer;
	_u32 _recvd_len;
}MEMBER_PROTOCAL_SEND_CALLBACK_ALL_ARG;


/*_int32 member_send_cmd_common_callback_change(MEMBER_SEND_CMD_RESP cb_fun,
	void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = SUCCESS;
	//EVENT switch_event = NULL;
	MEMBER_PROTOCAL_SEND_CALLBACK_ALL_ARG* data = NULL;
	
	ret = sd_malloc(sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_ALL_ARG), (void**)&data);
	CHECK_VALUE(ret);
	
	if(result!=0)
	{
		_int32 a = result;
		a = 0;
	}
	
	data->_user_data = user_data;
	data->_result = result;
	data->_recv_buffer = recv_buffer;
	data->_recvd_len = recvd_len;
	os_post_member_login_event(data, sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_ALL_ARG));
	//switch_event = create_event((WIDGET)member_get_wnd(), E_MEMBER_SEND_RESP, data, NULL);
	//return post_event(switch_event);
}*/

_int32 member_send_cmd_common_callback_change_handler(void *param)
{	
	_int32 ret = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_ALL_ARG* p_data = (MEMBER_PROTOCAL_SEND_CALLBACK_ALL_ARG*) param;
	
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA *p_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)p_data->_user_data;
	MEMBER_SEND_CMD_RESP cb_fun = p_user_data->_member_send_cmd_resp;
	MEMBER_PROTOCAL_IMPL * p_member = p_user_data->_p_member;
	
//	ret = member_send_cmd_common_callback(
//		p_data->_user_data,
//		p_data->_result,
//		p_data->_recv_buffer,
//		p_data->_recvd_len,
//		NULL,NULL);
	
	ret = cb_fun(p_member, p_data->_result, p_data->_recv_buffer, p_data->_recvd_len);
	
	sd_free(p_user_data);
	p_user_data = NULL;
	sd_free(p_data);
	p_data = NULL;
	
	return ret;
}

/* mini task 的回调函数 */
/*_int32 member_send_cmd_common_callback(void * user_data, _int32 result,
	void * recv_buffer, _u32 recvd_len, void * p_header,_u8 * send_data)
{
	_int32 ret = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA *p_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_SEND_CMD_RESP cb_fun = p_user_data->_member_send_cmd_resp;
	MEMBER_PROTOCAL_IMPL * p_member = p_user_data->_p_member;

//#if defined(SEND_CALLBACK_NOT_IN_THUNDER_INTERFACE_THREAD)
//	if(! ui_is_interface_thread())
//	{
		return member_send_cmd_common_callback_change((void *)member_send_cmd_common_callback,
			user_data, result, recv_buffer, recvd_len);
//	}
//#endif
//	
//	ret = cb_fun(p_member, result, recv_buffer, recvd_len);
	
//	sd_free(p_user_data);
	return ret;
}*/
/*
_int32 member_send_cmd_by_url_common_callback(ETM_HTTP_CALL_BACK * p_http_call_back_param)
{
	switch (p_http_call_back_param->_type)
	{
	case EHCT_PUT_RECVED_DATA:   //如果数据可能超出传进去的recv_buffer长度，需要进行多次接收合并操作
		{
			((MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)(p_http_call_back_param->_user_data))->_recv_buffer = p_http_call_back_param->_recved_data;
			((MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)(p_http_call_back_param->_user_data))->_recv_len = p_http_call_back_param->_recved_data_len;
		}
		break;
	case EHCT_NOTIFY_FINISHED:
		{
			member_send_cmd_common_callback(p_http_call_back_param->_user_data, p_http_call_back_param->_result,
				((MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)(p_http_call_back_param->_user_data))->_recv_buffer, 
				((MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)(p_http_call_back_param->_user_data))->_recv_len,
				p_http_call_back_param->_header,
				p_http_call_back_param->_sent_data);
		}
		break;
	case EHCT_NOTIFY_RESPN:
		{
			char * value = etm_get_http_header_value(p_http_call_back_param->_header, EHHV_STATUS_CODE);
			if (value != NULL)
			{
				value = NULL;
			
			}

			value = etm_get_http_header_value(p_http_call_back_param->_header, EHHV_CONTENT_LENGTH);
			if (value != NULL)
			{
				value = NULL;
			}
		}
		break;
	default:
		break;
	}
	return SUCCESS;
}
*/
static _int32 member_send_cmd_by_host(char *host, _u16 port, _u32 time_out,
	char *data, _u32 data_len, MEMBER_PROTOCAL_IMPL * p_member, MEMBER_SEND_CMD_RESP member_send_cmd_resp)
{
	_int32 ret = 0, send_ret = 0;
	char tmp_url[128];
	char *recv_buffer = NULL;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA *p_cb_user_data;

	EM_MINI_TASK task;
	sd_memset(&task, 0, sizeof(EM_MINI_TASK));

	LOG_DEBUG("member_send_cmd_by_host:host=%s:%u, time_out=%u, state=%u, data[%u]%s", host,port, time_out,p_member->_state,data_len, data);
	
	ret = sd_malloc(TMP_DATA_SIZE, (void **)&recv_buffer);
	CHECK_VALUE(ret);

	ret = sd_malloc(sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA), (void **)&p_cb_user_data);
	CHECK_VALUE(ret);

	sd_memset(p_cb_user_data, 0, sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA));
	p_cb_user_data->_p_member = p_member;
	p_cb_user_data->_member_send_cmd_resp = member_send_cmd_resp;
		
	sd_snprintf(tmp_url, 128, "http://%s:%u", host, port);
	task._url = tmp_url;
	task._url_len = sd_strlen(task._url);
	task._is_file = FALSE;
	task._send_data = (_u8*)data;
	task._send_data_len = data_len;
	task._recv_buffer = recv_buffer;
	task._recv_buffer_size = TMP_DATA_SIZE;
	task._callback_fun =(void*) member_send_cmd_resp;
	task._user_data = p_cb_user_data;
	task._timeout = time_out;
	send_ret = em_post_mini_file_to_url_impl(&task);

	if (send_ret != SUCCESS)
	{
		ret = sd_free(recv_buffer);
		recv_buffer = NULL;
		CHECK_VALUE(ret);
	}
	return send_ret;
}

static _int32 member_send_cmd_by_ip(MEMBER_SERVER_INFO *p_server, _u32 time_out,
	char *data, _u32 data_len, MEMBER_PROTOCAL_IMPL * p_member, MEMBER_SEND_CMD_RESP member_send_cmd_resp)
{
	_int32 ret = 0;
	char host[32] = {0};
	
	ret = sd_inet_ntoa(p_server->_ip, host, 32);
	CHECK_VALUE(ret);
	
	ret = member_send_cmd_by_host(host, p_server->_port, time_out, data, data_len, p_member,
		member_send_cmd_resp);
	return ret;
}

static _int32 member_send_cmd_by_url(char *url, _u32 time_out,
	char *data, _u32 data_len, MEMBER_PROTOCAL_IMPL * p_member, MEMBER_SEND_CMD_RESP member_send_cmd_resp)
{
	_int32 ret = 0, send_ret = 0;
	char *recv_buffer = NULL;
	EM_MINI_TASK http_post = {0};

	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA *p_cb_user_data;

	LOG_DEBUG("member_send_cmd_by_url:url=%s, time_out=%u, state=%u, data[%u]%s", url, time_out,p_member->_state,data_len, data);
	
	ret = sd_malloc(TMP_DATA_SIZE, (void **)&recv_buffer);
	CHECK_VALUE(ret);

	ret = sd_malloc(sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA), (void **)&p_cb_user_data);
	CHECK_VALUE(ret);

	sd_memset(p_cb_user_data, 0, sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA));
	p_cb_user_data->_p_member = p_member;
	p_cb_user_data->_member_send_cmd_resp = member_send_cmd_resp;

	http_post._url = url;
	http_post._url_len = sd_strlen(http_post._url);
	http_post._send_data = (_u8*)data;
	http_post._send_data_len = data_len;
	http_post._recv_buffer = recv_buffer;
	http_post._recv_buffer_size = TMP_DATA_SIZE;
	http_post._callback_fun = member_send_cmd_resp;
	http_post._user_data = p_cb_user_data;
	http_post._timeout = time_out;
	send_ret = em_post_mini_file_to_url_impl(&http_post);

	if (send_ret != SUCCESS)
	{
		ret = sd_free(recv_buffer);
		recv_buffer = NULL;
		CHECK_VALUE(ret);
	}
	return send_ret;
}

static _int32 member_send_cmd_get_file(char *url, char *file_path, _u32 file_path_len,
	char *file_name, _u32 file_name_len, MEMBER_PROTOCAL_IMPL * p_member, MEMBER_SEND_CMD_RESP member_send_cmd_resp)
{
	_int32 ret = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA *p_cb_user_data;
	EM_MINI_TASK task;

	LOG_DEBUG("member_send_cmd_get_file:url=%s, file_path[%u]%s, file_name[%u]=%s, state=%u", url,file_path_len, file_path,file_name_len,file_name,p_member->_state);
	
	ret = sd_malloc(sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA), (void **)&p_cb_user_data);
	CHECK_VALUE(ret);

	sd_memset(p_cb_user_data, 0, sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA));
	p_cb_user_data->_p_member = p_member;
	p_cb_user_data->_member_send_cmd_resp = member_send_cmd_resp;
	
	sd_memset(&task, 0, sizeof(EM_MINI_TASK));

	task._url = url;
	task._url_len = sd_strlen(task._url);
	task._is_file = TRUE;

	task._file_path_len = file_path_len;
	sd_strncpy(task._file_path, file_path, file_path_len);
	task._file_path[task._file_path_len] = '\0';
	
	task._file_name_len = file_name_len;
	sd_strncpy(task._file_name, file_name, file_name_len);
	task._file_name[task._file_name_len] = '\0';

	task._callback_fun = member_send_cmd_resp;
	task._user_data = p_cb_user_data;
	ret = em_get_mini_file_from_url_impl(&task);
	return ret;
}

static _int32 member_send_cmd_get_buffer(char *url,  char* recv_buf, _u32 recv_buf_size, _u32 timeout,MEMBER_PROTOCAL_IMPL * p_member, MEMBER_SEND_CMD_RESP member_send_cmd_resp)
{
	_int32 ret = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA *p_cb_user_data;
	EM_MINI_TASK task;

	LOG_DEBUG("member_send_cmd_get_buffer:url=%s, recv_buf_size=%u, state=%u", url,recv_buf_size,p_member->_state);
	
	ret = sd_malloc(sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA), (void **)&p_cb_user_data);
	CHECK_VALUE(ret);

	sd_memset(p_cb_user_data, 0, sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA));
	p_cb_user_data->_p_member = p_member;
	p_cb_user_data->_member_send_cmd_resp = member_send_cmd_resp;
	
	sd_memset(&task, 0, sizeof(EM_MINI_TASK));

	task._url = url;
	task._url_len = sd_strlen(task._url);

	task._recv_buffer = recv_buf;
	task._recv_buffer_size = recv_buf_size;

	task._callback_fun = member_send_cmd_resp;
	task._user_data = p_cb_user_data;
	task._timeout = timeout;
	ret = em_get_mini_file_from_url_impl(&task);
	return ret;
}


static BOOL member_need_query_portal(MEMBER_PROTOCAL_IMPL * p_member)
{
	if (p_member->_server_count == 1 && p_member->_server_index == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static BOOL member_need_query_vip_portal(MEMBER_PROTOCAL_IMPL * p_member)
{
	if (p_member->_vip_server_count == 1 && p_member->_vip_server_index == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static _int32 member_get_icon_file_name(MEMBER_PROTOCAL_IMPL * p_member, 
	char *file_name, _u32 size, _u32 *p_len)
{
	sd_memset(file_name, 0, size);
	sd_i64toa(p_member->_user_info._userno, file_name, size, 10);
	sd_strcat(file_name, ".jpg", sd_strlen(".jpg"));
	*p_len = sd_strlen(file_name);
	return SUCCESS;
}

// ------------------------------------------------------------------------

_int32 member_protocal_impl_init(MEMBER_PROTOCAL_IMPL * p_member, char *file_path, _u32 file_path_len,
	_u32 time_out)
{
	_int32 ret = SUCCESS;

	//char * main_server_host = "login3.reg2t.sandai.net";	// 外网
	char * main_server_host = DEFAULT_MAIN_MEMBER_SERVER_HOST_NAME ; //"phonelogin.reg2t.sandai.net";	// 外网(手机专用域名)
	_int32 main_server_port = 80;

	//char * portal_server_host = "portal.i.xunlei.com";	// 外网
	char * portal_server_host = DEFAULT_PORTAL_MEMBER_SERVER_HOST_NAME;  //"phoneportal.i.xunlei.com";	// 外网(手机专用域名)
	_int32 portal_server_port = 80;

	//char * vip_main_server_host = "cache2.vip.xunlei.com";	// 外网
	char * vip_main_server_host = DEFAULT_MAIN_VIP_SERVER_HOST_NAME;  //"phonecache.vip.xunlei.com";	// 外网(手机专用域名)
	_int32 vip_main_server_port = 8001;

	//char * vip_portal_server_host = "portal.i.xunlei.com";	// 外网
	char * vip_portal_server_host = DEFAULT_PORTAL_VIP_SERVER_HOST_NAME ; //"phoneportal.i.xunlei.com";	// 外网(手机专用域名)
	_int32 vip_portal_server_port = 80;

	LOG_DEBUG("--- init file_path=%s, timeout=%u", file_path, time_out);
	
	if (p_member == NULL)
	{
		return INVALID_ARGUMENT;
	}

	ret = sd_malloc(TMP_DATA_SIZE, (void **)&g_tmp_data);
	if (ret != SUCCESS)
	{
		g_tmp_data = NULL;
		return ret;
	}
	
	ret = sd_malloc(TMP_DATA_SIZE, (void **)&g_tmp_data_vip);
	if (ret != SUCCESS)
	{
		sd_free((void *)g_tmp_data);
		g_tmp_data = NULL;
		g_tmp_data_vip = NULL;
		return ret;
	}
	
	ret = sd_malloc(TMP_DATA_SIZE, (void **)&g_tmp_data_portal);
	if (ret != SUCCESS)
	{
		sd_free((void *)g_tmp_data);
		g_tmp_data = NULL;
		sd_free((void *)g_tmp_data_vip);
		g_tmp_data_vip = NULL;
		g_tmp_data_portal = NULL;
		return ret;
	}

	g_login_state = 0;
	g_querying_portal = FALSE;
	g_querying_vip_portal = FALSE;
	g_login_vip_state = 0;
	g_login_start_time_stamp = 0;
	g_login_max_time_out = 0;

#ifdef ENABLE_LOGIN_REPORT
	g_login_all_time = 0;
	g_login_query_vip_info = 0;
#endif
	
	ret = sd_memset(p_member, 0, sizeof(MEMBER_PROTOCAL_IMPL));
	
	// 从配置文件中读取主服务器。
	p_member->_main_server_host_len = sd_strlen(main_server_host);
	sd_strncpy(p_member->_main_server_host, main_server_host, p_member->_main_server_host_len);
	ret = em_settings_get_str_item("member.main_server_host", p_member->_main_server_host);

	ret = em_settings_get_int_item("member.main_server_port", &main_server_port);
	p_member->_main_server_port = (_u16)main_server_port;

	// 从配置文件中读取 portal 服务器。
	p_member->_portal_server_host_len = sd_strlen(portal_server_host);
	sd_strncpy(p_member->_portal_server_host, portal_server_host, p_member->_portal_server_host_len);
	ret = em_settings_get_str_item("member.portal_server_host", p_member->_portal_server_host);

	ret = em_settings_get_int_item("member.portal_server_port", &portal_server_port);
	p_member->_portal_server_port = (_u16)portal_server_port;

	// 从配置文件中读取 vip 主服务器。
	p_member->_vip_main_server_host_len = sd_strlen(vip_main_server_host);
	sd_strncpy(p_member->_vip_main_server_host, vip_main_server_host, p_member->_vip_main_server_host_len);
	ret = em_settings_get_str_item("member.vip_main_server_host", p_member->_vip_main_server_host);

	ret = em_settings_get_int_item("member.vip_main_server_port", &vip_main_server_port);
	p_member->_vip_main_server_port = (_u16)vip_main_server_port;
	
	// 从配置文件中读取 vip portal 服务器。
	p_member->_vip_portal_server_host_len = sd_strlen(vip_portal_server_host);
	sd_strncpy(p_member->_vip_portal_server_host, vip_portal_server_host, p_member->_vip_portal_server_host_len);
	ret = em_settings_get_str_item("member.vip_portal_server_host", p_member->_vip_portal_server_host);

	ret = em_settings_get_int_item("member.vip_portal_server_port", &vip_portal_server_port);
	p_member->_vip_portal_server_port = (_u16)vip_portal_server_port;

	// 初始化服务器个数。
	p_member->_server_count = 1;
	p_member->_server_index = 0;
	p_member->_vip_server_count = 1;
	p_member->_vip_server_index = 0;

	p_member->_asyn_time_out = time_out;

	p_member->_login_result_callback = NULL;
#if 0
	{
		// 测试服务器
		p_member->_servers[0]._ip = 0x96520E79; // 121.14.82.150
		//p_member->_servers[0]._ip = 0xF0520E79; // 121.14.82.240
		p_member->_servers[0]._port = 80;
	}
#endif

	// 初始化头像存储目录。
	if(file_path_len > 1)
	{
		if(!sd_dir_exist(file_path))
		{
			/* 头像目录如果不存在，先不设置头像目录 */
			return SUCCESS;
		}
		
		ret = sd_memset(p_member->_user_info._icon._file_path, 0, MEMBER_PROTOCAL_FILE_PATH_SIZE);
		CHECK_VALUE(ret);

		p_member->_user_info._icon._file_path_len = file_path_len;
		ret = sd_strncpy(p_member->_user_info._icon._file_path, file_path, file_path_len);
		CHECK_VALUE(ret);

		// 目录名称强制加'\\'
		if (p_member->_user_info._icon._file_path[p_member->_user_info._icon._file_path_len - 1] != '\\'
			&& p_member->_user_info._icon._file_path[p_member->_user_info._icon._file_path_len - 1] != '/')
		{
			p_member->_user_info._icon._file_path[p_member->_user_info._icon._file_path_len ] = DIR_SPLIT_CHAR;
			p_member->_user_info._icon._file_path[p_member->_user_info._icon._file_path_len+1] = '\0';
		}

		// 重新获取长度。
		p_member->_user_info._icon._file_path_len = sd_strlen(p_member->_user_info._icon._file_path);
	}

	LOG_DEBUG("init ok.");
	
	return SUCCESS;
}

_int32 member_protocal_impl_uninit(MEMBER_PROTOCAL_IMPL *p_member)
{
	LOG_DEBUG("--- uninit ok.");
	if (g_tmp_data != NULL)
	{
		sd_free((void *)g_tmp_data);
		g_tmp_data = NULL;
	}
	if (g_tmp_data_vip != NULL)
	{
		sd_free((void *)g_tmp_data_vip);
		g_tmp_data_vip = NULL;
	}
	if (g_tmp_data_portal!= NULL)
	{
		sd_free((void *)g_tmp_data_portal);
		g_tmp_data_portal = NULL;
	}

	g_login_state = 0;
	g_querying_portal = FALSE;
	g_querying_vip_portal = FALSE;
	g_login_vip_state = 0;
	g_login_start_time_stamp = 0;
	g_login_max_time_out = 0;
	
#ifdef ENABLE_LOGIN_REPORT
	g_login_all_time = 0;
	g_login_query_vip_info = 0;
#endif
	return SUCCESS;
}

_int32 member_protocal_impl_change_login_flag(_u32 new_flag)
{
	g_login_state = new_flag;

	LOG_DEBUG("member_protocal_impl_change_login_flag, new_flag=%u", new_flag);
	return SUCCESS;
}

_int32 member_protocal_impl_state_change(MEMBER_PROTOCAL_IMPL * p_member, _u32 new_state)
{
	_u32 old_state = p_member->_state;

	if (old_state == new_state)
	{
		return SUCCESS;
	}
	
	if (new_state == MEMBER_STATE_UNLOGIN)
	{
		//p_member->_server_index = 0;
		//p_member->_server_count = 1;
		
		//p_member->_vip_server_index = 0;
		//p_member->_vip_server_count = 1;
	}
	else if (new_state == MEMBER_STATE_LOGINING)
	{
	}
	else if (new_state == MEMBER_STATE_LOGINED)
	{
	}
	else if (new_state == MEMBER_STATE_PING)
	{
	}
	else if (new_state == MEMBER_STATE_REPORT_DOWNLOAD_FILE)
	{
	}
	else if (new_state == MEMBER_STATE_REFRESHING)
	{
	}
	else
	{
		// 状态错误。
		sd_assert(-1);
	}

	p_member->_state = new_state;

	LOG_DEBUG("change state, old=%u, new=%u", old_state, new_state);
	return SUCCESS;
}

// 用户登录（密码验证）。
_int32 member_protocal_impl_first_login(MEMBER_PROTOCAL_IMPL * p_member)
{
	g_login_max_time_out = 60*1000; /* 第一次登录，一分钟超时 */
	return member_protocal_impl_login( p_member);
}

#ifdef ENABLE_TCP_LOGIN
_int32 member_protocal_impl_http_login(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = 0;
	
	char url[1024] = {0};
	_u32 sendbuffer_len = 0;
	char *buffer = NULL; 
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA *p_cb_user_data;
	EM_MINI_TASK http_post;
	_u32 http_id = 0;
	
	if (p_member->_server_index == 0 && p_member->_servers[0]._ip == 0)
	{
		ret = sd_snprintf(url, sizeof(url), "http://%s:%u", DEFAULT_MAIN_MEMBER_SERVER_HOST_NAME, 80);
	}
	else
	{
		char host[32] = {0};
		ret = sd_inet_ntoa(p_member->_servers[p_member->_server_index]._ip, host, 32);
		if(ret!=SUCCESS)
		{
			g_login_vip_state = 3;
			CHECK_VALUE(ret);
		}
		ret = sd_snprintf(url, sizeof(url), "http://%s:%u", host, p_member->_servers[p_member->_server_index]._port);
	}

	LOG_DEBUG("member_protocal_impl_http_login, state=%u, srv_index=%u, addr=%u:%u, url=%s", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, url);
	
	sd_assert(ret <= 1024);

	ret = member_build_cmd_login(&p_member->_login_info, g_tmp_data, TMP_DATA_SIZE, &sendbuffer_len);
	CHECK_VALUE(ret);

	ret = sd_malloc(TMP_DATA_SIZE, (void**)&buffer);
	if(ret != SUCCESS)
	{
		LOG_DEBUG("member_protocal_impl_http_login: sd_malloc failed=%d\n", ret);
		CHECK_VALUE(ret);
	}
	
	ret = sd_malloc(sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA), (void **)&p_cb_user_data);
	if(ret!=SUCCESS)
	{
		SAFE_DELETE(buffer);
		CHECK_VALUE(ret);
	}
	sd_memset(p_cb_user_data, 0, sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA));
	p_cb_user_data->_p_member = p_member;
	p_cb_user_data->_member_send_cmd_resp = member_protocal_impl_login_callback;
	
	sd_memset(&http_post, 0, sizeof(EM_MINI_TASK));
	http_post._url = url;
	http_post._url_len = sd_strlen(http_post._url);

	http_post._send_data = g_tmp_data;
	http_post._send_data_len = sendbuffer_len;
	
	http_post._recv_buffer = buffer;
	http_post._recv_buffer_size = TMP_DATA_SIZE;

	http_post._callback_fun = member_protocal_impl_login_callback;
	http_post._user_data = p_cb_user_data;
	http_post._timeout = MEMBER_DEFAULT_LOGIN_TIMEOUT;
	ret = em_post_mini_file_to_url_impl(&http_post);
	if(ret!=SUCCESS)
	{
		SAFE_DELETE(buffer);
		SAFE_DELETE(p_cb_user_data);
		CHECK_VALUE(ret);
	}
	
	return SUCCESS;
}

_int32 member_protocal_impl_tcp_login(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret_val = 0;
	char *buffer = NULL;
	_u32 buffer_len = 0;
	_u32 had_send = 0;
	_u32 had_recv = 0;
	_u32 socket_id = 0;
	SD_SOCKADDR server_addr = {0};
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA *p_cb_user_data;
	
	LOG_DEBUG("member_protocal_impl_tcp_login, state=%u, srv_index=%u, addr=%u:%u", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port);
	
	ret_val = sd_create_socket(SD_AF_INET, SD_SOCK_STREAM, 0, &socket_id);
	if(SUCCESS != ret_val)
		goto ErrHandle;

	ret_val = sd_malloc(sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA), (void **)&p_cb_user_data);
	if(ret_val != SUCCESS)
		goto ErrHandle;
	sd_memset(p_cb_user_data, 0, sizeof(MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA));
	p_cb_user_data->_p_member = p_member;
	p_cb_user_data->_member_send_cmd_resp = member_protocal_impl_login_callback;

	if (p_member->_server_index == 0 && p_member->_servers[0]._ip == 0)
	{
		struct hostent *phostent = NULL;
		char str[32] = {0};
		phostent = gethostbyname(DEFAULT_MAIN_MEMBER_SERVER_HOST_NAME);
		char *addr = inet_ntop(phostent->h_addrtype, *(phostent->h_addr_list), str, sizeof(str));
		server_addr._sin_family = AF_INET;
		server_addr._sin_addr = sd_inet_addr(addr);
		server_addr._sin_port = sd_htons(80);
	}
	else
	{
		server_addr._sin_family = AF_INET;
		server_addr._sin_addr = p_member->_servers[p_member->_server_index]._ip;
		server_addr._sin_port = sd_htons(p_member->_servers[p_member->_server_index]._port);
	}
	
	ret_val = sd_connect(socket_id, &server_addr);
	if(SUCCESS != ret_val && WOULDBLOCK != ret_val)
	{
		LOG_DEBUG("member_protocal_impl_tcp_login: socket_proxy_connect_by_domain = %d\n", ret_val);
		goto ErrHandle;
	}
    /* Connecting success */
	ret_val = sd_malloc(16384, (void **)&buffer);
	if( SUCCESS != ret_val )
		goto ErrHandle;
	
	ret_val = member_build_cmd_login(&p_member->_login_info, buffer, 16384, &buffer_len);
	CHECK_VALUE(ret_val);
	if( buffer_len > 16384 )
	{
		ret_val = INVALID_ARGUMENT;
		goto ErrHandle;
	}	
	
	g_login_state = 1;
#ifdef LINUX
	fd_set rfds, wfds;
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_SET(socket_id, &rfds);
	FD_SET(socket_id, &wfds);
	struct timeval timeout;
	timeout.tv_sec = 2;
	timeout.tv_usec = 0;
	ret_val = select(socket_id + 1, NULL, &wfds, NULL, &timeout);
	if(ret_val > 0)
	{
		ret_val =  sd_send(socket_id, buffer, buffer_len, &had_send);
    	if( SUCCESS != ret_val && had_send != buffer_len)
		{
			ret_val = INVALID_SOCKET;
			goto ErrHandle;
		}
	}
	else
	{
		ret_val = INVALID_SOCKET;
		goto ErrHandle;
	}

	ret_val = select(socket_id + 1, &rfds, NULL, NULL, &timeout);
	if(ret_val > 0)
	{
		sd_memset(buffer, 0x00, 16384);
		ret_val =  sd_recv(socket_id, buffer, buffer_len, &had_recv);
    	if( SUCCESS != ret_val)
			goto ErrHandle;
	}
	else
	{
		ret_val = INVALID_SOCKET;
		goto ErrHandle;
	}
	
#else
	ret_val =  sd_send(socket_id, buffer, buffer_len, &had_send);
    if( SUCCESS != ret_val && had_send != buffer_len)
		goto ErrHandle;

	sd_memset(buffer, 0x00, 16384);
	ret_val =  sd_recv(socket_id, buffer, buffer_len, &had_recv);
    if( SUCCESS != ret_val)
		goto ErrHandle;
#endif

	p_cb_user_data->_recv_buffer = buffer;
	p_cb_user_data->_recv_len = had_recv;
	ret_val = SUCCESS;

ErrHandle:
	SAFE_DELETE(buffer);
	if( 0 != socket_id)
		sd_close_socket(socket_id);
	if( NULL != p_cb_user_data->_member_send_cmd_resp )
		p_cb_user_data->_member_send_cmd_resp(p_cb_user_data, ret_val, NULL, 0);
	return ret_val;
	
}


_int32 member_protocal_impl_login(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;

	if (p_member == NULL)
	{
		return INVALID_ARGUMENT;
	}

	LOG_DEBUG("member_protocal_impl_login--- start login, state=%u, srv_index=%u, addr=%u:%u, uname=%s", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_login_info._user_name);
	
	LOG_DEBUG("member_protocal_impl_login:g_login_state=%d!!",g_login_state);

	if( 1 == g_login_state)
	{
		sd_assert(g_login_state!=1);
		LOG_DEBUG("****member_protocal_impl_login: Logining... g_login_state=%d, ",g_login_state);
		return MEMBER_PROTOCAL_ERROR_OPER_DENIED;
	}

	if (p_member->_server_index >= p_member->_server_count)
	{
		LOG_DEBUG("****member_protocal_impl_login: ALL SERVER had logined.. g_login_state=%d, ",g_login_state);
		return MEMBER_PROTOCAL_ERROR_SERVER_ALL_FAIL;
	}

	if(g_login_max_time_out==0)
	{
	/* 非第一次登录，12秒超时 */
		g_login_max_time_out = 12*1000;
	 	LOG_DEBUG("member_protocal_impl_login: set timeout =%d", g_login_max_time_out);
	}
	
	if(p_member->_server_index==0 && g_http_login_state)
	{
		/* 记录时间戳 */
		sd_assert(g_login_start_time_stamp==0);
		sd_time_ms(&g_login_start_time_stamp);
		#ifdef ENABLE_LOGIN_REPORT
			sd_time_ms(&g_login_all_time);
		#endif
		LOG_DEBUG("member_protocal_impl_login: record start time =%u", g_login_start_time_stamp);
	}
	
	if(em_is_net_ok(TRUE)&&(sd_get_net_type()!=UNKNOWN_NET))
	{
		ret = SUCCESS;
		LOG_DEBUG("member_protocal_impl_login:net_type=%u!!",sd_get_net_type());
	}
	else
	{
		/* No network 回调通知 */
		LOG_DEBUG("member_protocal_impl_login:No network net_type=%u!!",sd_get_net_type());
		if( NULL != p_member->_login_result_callback )
			ret = p_member->_login_result_callback(NETWORK_NOT_READY, NULL, p_member);
		return SUCCESS;
	}

	ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_LOGINING);

	// 走tcp登录流程
	if(g_http_login_state)
	{ 
		ret = member_protocal_impl_tcp_login(p_member);
		if(SUCCESS == ret)
			return SUCCESS;
		else
			g_http_login_state = 0;
	}
	else
	{
		LOG_DEBUG("member_protocal_impl_login: http login");
		ret = member_build_cmd_login(&p_member->_login_info, g_tmp_data, TMP_DATA_SIZE, &cmd_len);
		CHECK_VALUE(ret);

		
		g_login_state = 1;
		if (p_member->_server_index == 0 && p_member->_servers[0]._ip == 0)
		{
			ret = member_send_cmd_by_host(p_member->_main_server_host, p_member->_main_server_port,
				p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
				member_protocal_impl_login_callback);
		}
		else
		{
			ret = member_send_cmd_by_ip(&p_member->_servers[p_member->_server_index], 
				p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
				member_protocal_impl_login_callback);
		}
		
		if (ret != SUCCESS)
		{
			// 发不出去。
			g_login_state = 3;
			//member_protocal_impl_state_change(p_member, MEMBER_STATE_UNLOGIN);
			return ret;
		}

		
		if (member_need_query_portal(p_member))
		{
			// 为加速登录,同时获取portal  ------Added by zyq @ 20121214
			ret = member_protocal_impl_query_portal(p_member);
			sd_assert(ret==SUCCESS);
			ret = member_protocal_impl_query_portal_vip(p_member);
			sd_assert(ret==SUCCESS);
		}
	}
	return SUCCESS;
}
#else
_int32 member_protocal_impl_login(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;

	if (p_member == NULL)
	{
		return INVALID_ARGUMENT;
	}

	LOG_DEBUG("member_protocal_impl_login--- start login, state=%u, srv_index=%u, addr=%u:%u, uname=%s", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_login_info._user_name);
	
	LOG_DEBUG("member_protocal_impl_login:g_login_state=%d!!",g_login_state);

	if( 1 == g_login_state)
	{
		sd_assert(g_login_state!=1);
		return MEMBER_PROTOCAL_ERROR_OPER_DENIED;
	}

	if (p_member->_server_index >= p_member->_server_count)
	{
		return MEMBER_PROTOCAL_ERROR_SERVER_ALL_FAIL;
	}

	if(g_login_max_time_out==0)
	{
	/* 非第一次登录，12秒超时 */
		g_login_max_time_out = 12*1000;
	}
	
	if(p_member->_server_index==0)
	{
		/* 记录时间戳 */
		sd_assert(g_login_start_time_stamp==0);
		sd_time_ms(&g_login_start_time_stamp);
		#ifdef ENABLE_LOGIN_REPORT
			sd_time_ms(&g_login_all_time);
		#endif
	}
	
	if(em_is_net_ok(TRUE)&&(sd_get_net_type()!=UNKNOWN_NET))
	{
		ret = SUCCESS;
		LOG_DEBUG("member_protocal_impl_login:net_type=%u!!",sd_get_net_type());
	}
	else
	{
		/* No network 回调通知 */
		LOG_DEBUG("member_protocal_impl_login:No network net_type=%u!!",sd_get_net_type());
		if( NULL != p_member->_login_result_callback )
			ret = p_member->_login_result_callback(NETWORK_NOT_READY, NULL, p_member);
		return SUCCESS;
	}

	ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_LOGINING);
	
	ret = member_build_cmd_login(&p_member->_login_info, g_tmp_data, TMP_DATA_SIZE, &cmd_len);
	CHECK_VALUE(ret);

	g_login_state = 1;
	if (p_member->_server_index == 0 && p_member->_servers[0]._ip == 0)
	{
		ret = member_send_cmd_by_host(p_member->_main_server_host, p_member->_main_server_port,
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_login_callback);
	}
	else
	{
		ret = member_send_cmd_by_ip(&p_member->_servers[p_member->_server_index], 
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_login_callback);
	}
	
	if (ret != SUCCESS)
	{
		// 发不出去。
		g_login_state = 3;
		//member_protocal_impl_state_change(p_member, MEMBER_STATE_UNLOGIN);
		return ret;
	}

	
	
	if (member_need_query_portal(p_member))
	{
		// 为加速登录,同时获取portal  ------Added by zyq @ 20121214
		ret = member_protocal_impl_query_portal(p_member);
		sd_assert(ret==SUCCESS);
		ret = member_protocal_impl_query_portal_vip(p_member);
		sd_assert(ret==SUCCESS);
	}
	return SUCCESS;
}
#endif
_int32 member_protocal_impl_sessionid_login(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = 0;
	_u32 cmd_len = 0; /* request package len */

	if (p_member == NULL)
	{
		return INVALID_ARGUMENT;
	}
	g_login_max_time_out = 60*1000; /* 第一次登录，一分钟超时 */

	LOG_DEBUG("member_protocal_impl_sessionid_login--- start login, state=%u, srv_index=%u, addr=%u:%u, sessionid=%s", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_session_login_info._sessionid);
	
	LOG_DEBUG("member_protocal_impl_sessionid_login:g_login_state=%d!!",g_login_state);
	sd_assert(g_login_state!=1);

	if (p_member->_server_index >= p_member->_server_count)
	{
		return MEMBER_PROTOCAL_ERROR_SERVER_ALL_FAIL;
	}

	if(p_member->_server_index==0)
	{
		/* 记录时间戳 */
		sd_assert(g_login_start_time_stamp==0);
		sd_time_ms(&g_login_start_time_stamp);
	}
	
	if(em_is_net_ok(TRUE)&&(sd_get_net_type()!=UNKNOWN_NET))
	{
		ret = SUCCESS;
		LOG_DEBUG("member_protocal_impl_sessionid_login:net_type=%u!!",sd_get_net_type());
	}
	else
	{
		/* No network 回调通知 */
		LOG_DEBUG("member_protocal_impl_sessionid_login:No network net_type=%u!!",sd_get_net_type());
		if( NULL != p_member->_login_result_callback )
			ret = p_member->_login_result_callback(NETWORK_NOT_READY, NULL, p_member);
		return SUCCESS;
	}

	ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_LOGINING);

	sd_memset(g_tmp_data, 0x00, TMP_DATA_SIZE);
	ret = member_build_cmd_sessionid_login(&p_member->_session_login_info, g_tmp_data, TMP_DATA_SIZE, &cmd_len);
	CHECK_VALUE(ret);

	g_login_state = 1;
	
	if (p_member->_server_index == 0 && p_member->_servers[0]._ip == 0)
	{
		ret = member_send_cmd_by_host(p_member->_main_server_host, p_member->_main_server_port,
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_sessionid_login_callback);
	}
	else
	{
		ret = member_send_cmd_by_ip(&p_member->_servers[p_member->_server_index], 
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_sessionid_login_callback);
	}

	
	if (ret != SUCCESS)
	{
		// 发不出去。
		g_login_state = 3;
		LOG_ERROR("member_protocal_impl_sessionid_login: network is bad, g_login_state=%u!!",g_login_state);
		//member_protocal_impl_state_change(p_member, MEMBER_STATE_UNLOGIN);
		return ret;
	}
	else if (member_need_query_portal(p_member))
	{
		// 为加速登录,同时获取portal  ------Added by zyq @ 20121214
		ret = member_protocal_impl_query_portal(p_member);
		sd_assert(ret==SUCCESS);
		ret = member_protocal_impl_query_portal_vip(p_member);
		sd_assert(ret==SUCCESS);
	}
	return SUCCESS;
}

_int32 member_protocal_impl_sessionid_login_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	_int32 query_result = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL* p_member = p_cb_user_data->_p_member;
	char *data = (char *)recv_buffer;
	_u32 data_len = recvd_len;
	_u32 cur_time_stamp = 0;

	DEAL_WITH_LOGIN_RESULT_CALLBACK login_result_callback = p_member->_login_result_callback;

	LOG_DEBUG("member_protocal_impl_sessionid_login_callback, state=%u, srv_index=%u, addr=%u:%u, result=%u, sessionid=%s", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, result, p_member->_session_login_info._sessionid);
	
	g_login_state = 2;
	
	if (result == SUCCESS)
	{
		// 登录成功,解析sessionid登录结果。
		query_result = member_parse_cmd_sessionid_login_resp(data, data_len, &p_member->_user_info);

		LOG_DEBUG("member_protocal_impl_sessionid_login_callback, state=%u, srv_index=%u, addr=%u:%u, query_result=%u, uid=%llu", p_member->_state,
			p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
			p_member->_servers[p_member->_server_index]._port, query_result, p_member->_user_info._userno);
		
		SAFE_DELETE(recv_buffer);
		SAFE_DELETE(p_cb_user_data);
		
		if (query_result == SUCCESS)
		{
			// 设置信息到全局g_member_info
			member_protocal_interface_login_basic_resp_impl(&p_member->_user_info);
			// 直接开始查VIP信息。
			ret = member_protocal_impl_login_vip(p_member);
			if (ret != SUCCESS)
			{
				// 发送失败了，回调通知。
				if( NULL != login_result_callback )
				{	
					ret = login_result_callback(ret, NULL, p_member);
					sd_assert(ret==SUCCESS);
				}
			}
			else
			{
				// 开始获取用户信息。
				ret = member_protocal_impl_query_user_info(p_member);
				sd_assert(ret==SUCCESS);
				
				// 获取头像。
				ret = member_protocal_impl_query_icon(p_member);
				sd_assert(ret==SUCCESS);
			}
			
			return SUCCESS;
		}
		else if (query_result == MEMBER_PROTOCAL_ERROR_LOGIN_SYSTEM_MAINTENANCE
			|| query_result == MEMBER_PROTOCAL_ERROR_LOGIN_SERVER_INNER_ERROR)
		{
			// 需要查下一个server.
			g_login_state = 0;
		}
		else
		{
			// 服务器验证失败，回调通知。
			g_login_state = 3;
			if( NULL != login_result_callback )
				ret = login_result_callback(query_result, NULL, p_member);
			return SUCCESS;
		}
	}
	else
	{
		SAFE_DELETE(recv_buffer);
		SAFE_DELETE(p_cb_user_data);
		g_login_state = 0;
	}
	/* 多次尝试登录超出时间限制，则不再进行登录，回调界面 */
	sd_time_ms(&cur_time_stamp);
	if(TIME_SUBZ(cur_time_stamp, g_login_start_time_stamp) > g_login_max_time_out)
	{
		g_login_state = 3;
		if( NULL != login_result_callback )
			ret = login_result_callback(-1, NULL, p_member);
		return SUCCESS;
	}
	
	// 本服务器有问题，尝试下一个。
	if (member_need_query_portal(p_member))
	{
		// 获取portal
		sd_assert(g_login_state == 0);
		ret = member_protocal_impl_query_portal(p_member);
		if (ret != SUCCESS)
		{
			// 发送失败了，回调通知。
			g_login_state = 3;
			if( NULL != login_result_callback )
				ret = login_result_callback(ret, NULL, p_member);
		}
		
		return SUCCESS;
	}
	
	// 下一个服务器
	p_member->_server_index ++;
	if (p_member->_server_index < p_member->_server_count)
	{
		// 重试下一个服务器
		sd_assert(g_login_state == 0);
		ret = member_protocal_impl_sessionid_login(p_member);
		if (ret != SUCCESS)
		{
			// 发送失败了，回调通知。
			g_login_state = 3;
			if( NULL != login_result_callback )
				ret = login_result_callback(ret, NULL, p_member);
		}
		return SUCCESS;
	}
	else
	{
		p_member->_server_index = 0;
		p_member->_server_count = 1;
	}
	
	// 全部服务器都失败了，回调通知。
	g_login_state = 3;
	if( NULL != login_result_callback )
		ret = login_result_callback(MEMBER_PROTOCAL_ERROR_SERVER_ALL_FAIL, NULL, p_member);

	return SUCCESS;
}

_int32 member_protocal_impl_login_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	_int32 query_result = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL* p_member = p_cb_user_data->_p_member;
	char *data = (char *)recv_buffer;
	_u32 data_len = recvd_len;
	_u32 cur_time_stamp = 0;

	DEAL_WITH_LOGIN_RESULT_CALLBACK login_result_callback = p_member->_login_result_callback;
#ifdef ENABLE_TCP_LOGIN
	LOG_DEBUG("member_protocal_impl_login_callback, login_type = %d", g_http_login_state);
#endif
	LOG_DEBUG("member_protocal_impl_login_callback, state=%u, srv_index=%u, addr=%u:%u, result=%d, uname=%s", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, result, p_member->_login_info._user_name);
	g_login_state = 2;

	#ifdef ENABLE_LOGIN_REPORT
		sd_time_ms(&cur_time_stamp);
		_u32 response_time = TIME_SUBZ(cur_time_stamp, g_login_all_time);
		member_report_login_info(0x0001, p_member, result, response_time);
	#endif

	if (result == SUCCESS)
	{
		query_result = member_parse_cmd_login_resp(data, data_len, &p_member->_user_info);

		LOG_DEBUG("login_callback, state=%u, srv_index=%u, addr=%u:%u, query_result=%u, uid=%llu", p_member->_state,
			p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
			p_member->_servers[p_member->_server_index]._port, query_result, p_member->_user_info._userno);
		
		SAFE_DELETE(recv_buffer);
		SAFE_DELETE(p_cb_user_data);
		
		if (query_result == SUCCESS)
		{
			member_protocal_interface_login_basic_resp_impl(&p_member->_user_info);
			
			// 密码验证成功，直接开始查VIP信息。
			ret = member_protocal_impl_login_vip(p_member);
			if (ret != SUCCESS)
			{
				// 发送失败了，回调通知。
				if( NULL != login_result_callback )
					ret = login_result_callback(ret, NULL, p_member);
				sd_assert(ret==SUCCESS);
			}
			else
			{
				// 密码验证成功，开始获取用户信息。
				ret = member_protocal_impl_query_user_info(p_member);
				sd_assert(ret==SUCCESS);
				
				// 获取头像。
				ret = member_protocal_impl_query_icon(p_member);
				sd_assert(ret==SUCCESS);
			}
			
			return SUCCESS;
		}
		else if (query_result == MEMBER_PROTOCAL_ERROR_LOGIN_SYSTEM_MAINTENANCE
			|| query_result == MEMBER_PROTOCAL_ERROR_LOGIN_SERVER_INNER_ERROR)
		{
			// 需要查下一个server.
			g_login_state = 0;
		}
		else
		{
			// 服务器验证失败，回调通知。
			g_login_state = 3;
			if( NULL != login_result_callback )
				ret = login_result_callback(query_result, NULL, p_member);
			return SUCCESS;
		}
	}
	else
	{
		SAFE_DELETE(recv_buffer);
		SAFE_DELETE(p_cb_user_data);
		g_login_state = 0;

		if( 0 == p_member->_server_index )
		{
			// 测试网络信息上报
			member_start_test_network(result, 1);
		}
		
	#ifdef ENABLE_TCP_LOGIN
		if(g_http_login_state)
		{
			g_http_login_state = 0;
			LOG_DEBUG("member_protocal_impl_login_callback, change login_type = %d, tcp-->http", g_http_login_state);
			ret = member_protocal_impl_login(p_member);
			if(SUCCESS == ret)
				return SUCCESS;
		}
	#endif
	}

	sd_time_ms(&cur_time_stamp);
	if(TIME_SUBZ(cur_time_stamp, g_login_start_time_stamp)>g_login_max_time_out)
	{
		/* 超时,回调通知。 */
		g_login_state = 3;
		LOG_DEBUG("member_protocal_impl_login_callback: timeout!!! timeout_time= %d", g_login_max_time_out);
		if( NULL != login_result_callback )
			ret = login_result_callback(-1, NULL, p_member);
		return SUCCESS;
	}
	
	// 本服务器有问题，尝试下一个。
	if (member_need_query_portal(p_member))
	{
		// 获取portal
		sd_assert(g_login_state == 0);
		ret = member_protocal_impl_query_portal(p_member);
		if (ret != SUCCESS)
		{
			// 发送失败了，回调通知。
			g_login_state = 3;
			if( NULL != login_result_callback )
				ret = login_result_callback(ret, NULL, p_member);
		}
		
		return SUCCESS;
	}
	
	// 下一个服务器
	p_member->_server_index ++;
	if (p_member->_server_index < p_member->_server_count)
	{
		// 重试下一个服务器
		//sd_assert(g_login_state == 0);
		g_login_state == 0;
	#ifdef ENABLE_TCP_LOGIN
		g_http_login_state = 1;
		LOG_DEBUG("member_protocal_impl_login_callback, try next server: change login_type = %d, http-->tcp", g_http_login_state);
	#endif
		ret = member_protocal_impl_login(p_member);
		if (ret != SUCCESS)
		{
			// 发送失败了，回调通知。
			g_login_state = 3;
			LOG_DEBUG("member_protocal_impl_login_callback, FAILED: g_login_state = %d", g_login_state);
			if( NULL != login_result_callback )
				ret = login_result_callback(ret, NULL, p_member);
		}

		return SUCCESS;
	}
	else
	{
		p_member->_server_index = 0;
		p_member->_server_count = 1;
	}
	
	// 全部服务器都失败了，回调通知。
	g_login_state = 3;
	LOG_DEBUG("member_protocal_impl_login_callback, ALL SERVER FAILED: g_login_state = %d", g_login_state);
	if( NULL != login_result_callback )
		ret = login_result_callback(MEMBER_PROTOCAL_ERROR_SERVER_ALL_FAIL, NULL, p_member);

	return SUCCESS;
}

// 获取用户信息。
_int32 member_protocal_impl_query_user_info(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;
	
	LOG_DEBUG("member_protocal_impl_query_user_info, state=%u, srv_index=%u, addr=%u:%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_user_info._userno);

	if (p_member->_server_index >= p_member->_server_count)
	{
		sd_assert(FALSE);
		return MEMBER_PROTOCAL_ERROR_SERVER_FAIL;
	}
	
	ret = member_build_cmd_query_user_info(&p_member->_user_info, g_tmp_data, TMP_DATA_SIZE, &cmd_len);
	CHECK_VALUE(ret);

	#ifdef ENABLE_LOGIN_REPORT
		sd_time_ms(&g_login_query_vip_info);
	#endif

	if (p_member->_server_index == 0 && p_member->_servers[0]._ip == 0)
	{
		ret = member_send_cmd_by_host(p_member->_main_server_host, p_member->_main_server_port,
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_query_user_info_callback);
	}
	else
	{
		ret = member_send_cmd_by_ip(&p_member->_servers[p_member->_server_index], 
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_query_user_info_callback);
	}
	
	return ret;
}

_int32 member_protocal_impl_query_user_info_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	_int32 query_result = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL* p_member = p_cb_user_data->_p_member;
	char *data = (char *)recv_buffer;
	_u32 data_len = recvd_len;

	LOG_DEBUG("member_protocal_impl_query_user_info_callback, state=%u, srv_index=%u, addr=%u:%u, result=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, result, p_member->_user_info._userno);

	#ifdef ENABLE_LOGIN_REPORT
		_u32 cur_time_stamp = 0;
		sd_time_ms(&cur_time_stamp);
		_u32 response_time = TIME_SUBZ(cur_time_stamp, g_login_query_vip_info);
		member_report_login_info(0x0003, p_member, result, response_time);
	#endif

	if(g_login_state!=2)
	{
		SAFE_DELETE(recv_buffer);
		SAFE_DELETE(p_cb_user_data);
		return SUCCESS;
	}

	SAFE_DELETE(p_cb_user_data);
	
	if (result == SUCCESS)
	{
		query_result = member_parse_cmd_query_user_info_resp(data, data_len, &p_member->_user_info);
		LOG_DEBUG("query_user_info_callback, state=%u, srv_index=%u, addr=%u:%u, query_result=%u, uid=%llu", p_member->_state,
			p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
			p_member->_servers[p_member->_server_index]._port, query_result, p_member->_user_info._userno);
		
		SAFE_DELETE(recv_buffer);

		member_protocal_interface_user_info_resp(SUCCESS,&p_member->_user_info);
		
		return SUCCESS;
	}
	else
	{
		SAFE_DELETE(recv_buffer);
	}
	
	return SUCCESS;
}

_int32 member_protocal_impl_query_portal(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;

	LOG_DEBUG("member_protocal_impl_query_portal, state=%u, srv_index=%u, addr=%u:%u, uid=%llu,portal_server._ip=0x%X,portal_server_host=%s:%u,asyn_time_out=%u", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_user_info._userno,p_member->_portal_server._ip,p_member->_portal_server_host,p_member->_main_server_port,p_member->_asyn_time_out);

	LOG_ERROR("member_protocal_impl_query_portal:%d",g_querying_portal);
	if(g_querying_portal)
	{
		return SUCCESS;
	}

	g_querying_portal = TRUE;
	
	ret = member_build_cmd_query_portal(g_tmp_data_portal, TMP_DATA_SIZE, &cmd_len, p_member);
	if(ret!=SUCCESS)
	{
		g_querying_portal = FALSE;
		CHECK_VALUE(ret);
	}

	if (p_member->_portal_server._ip == 0)
	{
		ret = member_send_cmd_by_host(p_member->_portal_server_host, p_member->_main_server_port,
			p_member->_asyn_time_out, g_tmp_data_portal, cmd_len, p_member,
			member_protocal_impl_query_portal_callback);
	}
	else
	{
		ret = member_send_cmd_by_ip(&p_member->_portal_server, 
			p_member->_asyn_time_out, g_tmp_data_portal, cmd_len, p_member,
			member_protocal_impl_query_portal_callback);
	}

	if(ret!=SUCCESS)
	{
		g_querying_portal = FALSE;
		CHECK_VALUE(ret);
	}
	return SUCCESS;
}

_int32 member_protocal_impl_query_portal_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	_int32 query_result = 0;
	_u32 bak_server_count = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL* p_member = p_cb_user_data->_p_member;
	char *data = (char *)recv_buffer;
	_u32 data_len = recvd_len;

	DEAL_WITH_LOGIN_RESULT_CALLBACK login_result_callback = p_member->_login_result_callback;

	LOG_DEBUG("member_protocal_impl_query_portal_callback, state=%u, srv_index=%u, addr=%u:%u, result=%d, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, result, p_member->_user_info._userno);
	
	if(g_querying_portal)
	{
		g_querying_portal = FALSE;
	}
	else
	{
		/* 已被取消 */
		SAFE_DELETE(recv_buffer);
		SAFE_DELETE(p_cb_user_data);
		return SUCCESS;
	}

	if (result != SUCCESS)
	{
		if(g_login_state==0)
		{
			// 服务器问题，回调通知。
			if( NULL != login_result_callback )
				ret = login_result_callback(result, NULL, p_member);
		}
		SAFE_DELETE(recv_buffer);
		SAFE_DELETE(p_cb_user_data);

		// 测试网络信息上报
		member_start_test_network(result, 3);
		
		return SUCCESS;
	}
	
	query_result = member_parse_cmd_query_portal_resp(data, data_len, &p_member->_servers[1], MEMBER_SERVER_MAX_COUNT-1,
		&bak_server_count);

	LOG_DEBUG("member_protocal_impl_query_portal_callback, state=%u, srv_index=%u, addr=%u:%u, query_result=%u, bak_srv_count=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, query_result, bak_server_count, p_member->_user_info._userno);
	
	SAFE_DELETE(recv_buffer);
	SAFE_DELETE(p_cb_user_data);

	
	if (query_result == SUCCESS && bak_server_count > 0 )
	{
		p_member->_server_count = bak_server_count + 1;
		if(g_login_state==0)
		{
			// 开始查 bak server。
			p_member->_server_index = 1;
			LOG_DEBUG("member_protocal_impl_query_portal_callback, start login  g_login_state = %d", g_login_state);
			if(p_member->_login_info._user_name_len != 0 && p_member->_login_info._passwd_len != 0)
				ret = member_protocal_impl_login(p_member);
			else if(p_member->_session_login_info._sessionid_len != 0)
				ret = member_protocal_impl_sessionid_login(p_member);
			else
				ret = INVALID_ARGUMENT;
			if (ret != SUCCESS)
			{
				// 发送失败了，回调通知。
				if( NULL != login_result_callback )
					ret = login_result_callback(ret, NULL, p_member);
			}
		}
		return SUCCESS;
	}

	if(g_login_state==0)
	{
		// 查询失败了，回调通知。
		if(query_result!=SUCCESS)
		{
			if( NULL != login_result_callback )
				ret = login_result_callback(query_result, NULL, p_member);
		}
		else
		{
			if( NULL != login_result_callback )
				ret = login_result_callback(MEMBER_PROTOCAL_ERROR_QUERY_LOGIN_PORTAL_FAIL, NULL, p_member);
		}
	}
	return SUCCESS;
}

_int32 member_protocal_impl_login_vip(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;
	char url[1024] = {0};
	char *buffer = NULL;
	
	LOG_DEBUG("login_vip, state=%u, srv_index=%u, addr=%u:%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_user_info._userno);

	if(p_member->_vip_server_index >= p_member->_vip_server_count)
	{
		return MEMBER_PROTOCAL_ERROR_SERVER_ALL_VIP_FAIL;
	}
	
	g_login_vip_state = 1;
	
#if 1
	if(sd_strlen(p_member->_user_info._session_id)==0)
	{	
		LOG_DEBUG("member_protocal_impl_login_vip: sessionid is NULL\n");
		CHECK_VALUE(MEMBER_PROTOCAL_ERROR_NO_SECSSION_ID);
	}
	
	LOG_DEBUG("member_protocal_impl_login_vip: userid=%llu, sessionid=%s\n", p_member->_user_info._userno, p_member->_user_info._session_id);

	if (p_member->_vip_server_index == 0 && p_member->_vip_servers[0]._ip == 0)
	{
		ret = sd_snprintf(url, sizeof(url), "http://%s:%u/cache?userid=%llu&protocol_version=%d&secure=false&sessionid=%s", MEMBER_VIP_HTTP_SERVER_HOST, MEMBER_VIP_HTTP_SERVER_PORT, p_member->_user_info._userno, QUERY_VIP_PROTOCOL_VERSION, p_member->_user_info._session_id);
	}
	else
	{
		char host[32] = {0};
		
		ret = sd_inet_ntoa(p_member->_vip_servers[p_member->_vip_server_index]._ip, host, 32);
		if(ret!=SUCCESS)
		{
			g_login_vip_state = 3;
			CHECK_VALUE(ret);
		}
		ret = sd_snprintf(url, sizeof(url), "http://%s:%u/cache?userid=%llu&protocol_version=%d&secure=false&sessionid=%s", host, p_member->_vip_servers[p_member->_vip_server_index]._port, p_member->_user_info._userno,QUERY_VIP_PROTOCOL_VERSION, p_member->_user_info._session_id);
	}
	LOG_DEBUG("member_protocal_impl_login_vip: url=%s\n", url);
	sd_assert(ret <= 1024);

	ret = sd_malloc(16384, (void**)&buffer);
	if(ret!=SUCCESS)
	{
		g_login_vip_state = 3;
		CHECK_VALUE(ret);
	}

	ret = member_send_cmd_get_buffer(url, buffer, 16384, p_member->_asyn_time_out,p_member, member_protocal_impl_login_vip_callback);
	if(ret!=SUCCESS)
	{
		g_login_vip_state = 3;
		SAFE_DELETE(buffer);
		CHECK_VALUE(ret);
	}

#else	
	ret = member_build_cmd_login_vip(&p_member->_user_info, g_tmp_data_vip, TMP_DATA_SIZE, &cmd_len);
	if(ret!=SUCCESS)
	{
		g_login_vip_state = 3;
		CHECK_VALUE(ret);
	}

	if (p_member->_vip_server_index == 0 && p_member->_vip_servers[0]._ip == 0)
	{
		ret = member_send_cmd_by_host(p_member->_vip_main_server_host, p_member->_vip_main_server_port,
			MAX_VIP_LOGIN_TIMEOUT, g_tmp_data_vip, cmd_len, p_member,
			member_protocal_impl_login_vip_callback);
	}
	else
	{
		ret = member_send_cmd_by_ip(&p_member->_vip_servers[p_member->_vip_server_index], 
			MAX_VIP_LOGIN_TIMEOUT, g_tmp_data_vip, cmd_len, p_member,
			member_protocal_impl_login_vip_callback);
	}

	if(ret!=SUCCESS)
	{
		g_login_vip_state = 3;
		CHECK_VALUE(ret);
	}
#endif
	
	return SUCCESS;
}

_int32 member_protocal_impl_login_vip_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	_int32 query_result = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL* p_member = p_cb_user_data->_p_member;
	char *data = (char *)recv_buffer;
	_u32 data_len = recvd_len;
	_u32 cur_time_stamp = 0;
	char expired_time[] = "未知";
	char utf8_time[8] = {0};
	_u32 utf8_len = 8;

	DEAL_WITH_LOGIN_RESULT_CALLBACK login_result_callback = p_member->_login_result_callback;

	LOG_DEBUG("member_protocal_impl_login_vip_callback, state=%u, srv_index=%u, addr=%u:%u, result=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, result, p_member->_user_info._userno);

	if(g_login_state!=2)
	{
		g_login_vip_state = 3;
		SAFE_DELETE(recv_buffer);
		SAFE_DELETE(p_cb_user_data);
		return SUCCESS;
	}
	
	SAFE_DELETE(p_cb_user_data);

	if (result == SUCCESS)
	{
#if 1
		query_result = member_parse_http_query_vip_resp(data, data_len, &p_member->_user_info);
		// 会员企业账号的子账号强制定义为会员
		if(  p_member->_user_info._vas&0x10000000 )
		{	
		#if defined(CLOUD_PLAY_PROJ) 
			p_member->_user_info._is_vip = TRUE;
			p_member->_user_info._is_platinum = TRUE;
		#endif
            sd_memset(p_member->_user_info._expire_date, 0x00, sizeof(p_member->_user_info._expire_date));
			sd_any_format_to_utf8(expired_time, sd_strlen(expired_time), utf8_time, &utf8_len);
			sd_strncpy(p_member->_user_info._expire_date, utf8_time, utf8_len);
            p_member->_user_info._expire_date_len = utf8_len;
		}
#else
		query_result = member_parse_cmd_login_vip_resp(data, data_len, &p_member->_user_info);

		LOG_DEBUG("login_vip_callback, state=%u, srv_index=%u, addr=%u:%u, query_result=%u, uid=%llu", p_member->_state,
			p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
			p_member->_servers[p_member->_server_index]._port, query_result, p_member->_user_info._userno);
#endif
		SAFE_DELETE(recv_buffer);
		if (query_result == SUCCESS)
		{
			//查询VIP信息成功
			g_login_vip_state = 2;
			ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_LOGINED);
			if( NULL != login_result_callback )
				ret = login_result_callback(SUCCESS, &p_member->_user_info, p_member);

			return SUCCESS;
		}
		else
		{
			// 登录失败了，回调通知。
			g_login_vip_state= 3;
			if( NULL != login_result_callback )
				ret = login_result_callback(query_result, NULL, p_member);

			return SUCCESS;
		}
	}
	else
	{
		SAFE_DELETE(recv_buffer);
		g_login_vip_state = 0;
		//return SUCCESS;
		if(0 == p_member->_vip_server_index)
		{	
			// 测试网络信息上报
			member_start_test_network(result, 2);
		}
	}


	sd_time_ms(&cur_time_stamp);
	if(TIME_SUBZ(cur_time_stamp, g_login_start_time_stamp)>g_login_max_time_out)
	{
		/*  */
		g_login_state = 3;
		if( NULL != login_result_callback )
			ret = login_result_callback(-1, NULL, p_member);

		return SUCCESS;
	}
	
	// 本服务器有问题，尝试下一个。
	if (member_need_query_vip_portal(p_member))
	{
		// 获取portal
		sd_assert(g_login_vip_state==0);
		ret = member_protocal_impl_query_portal_vip(p_member);
		if (ret != SUCCESS)
		{
			// 发送失败了，回调通知。
			g_login_vip_state= 3;
			if( NULL != login_result_callback )
				ret = login_result_callback(ret, NULL, p_member);
		}
		return SUCCESS;
	}
	
	// 下一个服务器
	p_member->_vip_server_index ++;
	if (p_member->_vip_server_index < p_member->_vip_server_count)
	{
		// 重试下一个服务器
		sd_assert(g_login_vip_state==0);
		ret = member_protocal_impl_login_vip(p_member);
		if (ret != SUCCESS)
		{
			// 发送失败了，回调通知。
			g_login_vip_state= 3;
			if( NULL != login_result_callback )
				ret = login_result_callback(ret, NULL, p_member);
		}
		return SUCCESS;
	}
	else
	{
		/* 全部服务器都失败了，下次登录从主服务器开始试 */
		p_member->_vip_server_index = 0;
		p_member->_vip_server_count = 1;
		g_login_vip_state = 3;
	}

	// 全部服务器都失败了，回调通知。
	if( NULL != login_result_callback )
		ret = login_result_callback(MEMBER_PROTOCAL_ERROR_SERVER_ALL_VIP_FAIL, NULL, p_member);

	return SUCCESS;
}

_int32 member_protocal_impl_query_portal_vip(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;

	LOG_DEBUG("member_protocal_impl_query_portal_vip, state=%u, srv_index=%u, addr=%u:%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_user_info._userno);

	LOG_ERROR("member_protocal_impl_query_portal_vip:%d",g_querying_vip_portal);
	if(g_querying_vip_portal)
	{
		return SUCCESS;
	}

	g_querying_vip_portal = TRUE;

	ret = member_build_cmd_query_portal_vip(g_tmp_data_vip, TMP_DATA_SIZE, &cmd_len, p_member);
	if(ret!=SUCCESS)
	{
		g_querying_vip_portal = FALSE;
		CHECK_VALUE(ret);
	}

	if (p_member->_vip_portal_server._ip == 0)
	{
		ret = member_send_cmd_by_host(p_member->_vip_portal_server_host, p_member->_vip_portal_server_port,
			p_member->_asyn_time_out, g_tmp_data_vip, cmd_len, p_member,
			member_protocal_impl_query_portal_vip_callback);
	}
	else
	{
		ret = member_send_cmd_by_ip(&p_member->_vip_portal_server,
			p_member->_asyn_time_out, g_tmp_data_vip, cmd_len, p_member,
			member_protocal_impl_query_portal_vip_callback);
	}
	
	if(ret!=SUCCESS)
	{
		g_querying_vip_portal = FALSE;
		CHECK_VALUE(ret);
	}
	return ret;
}

_int32 member_protocal_impl_query_portal_vip_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	_int32 query_result = 0;
	_u32 bak_server_count = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL* p_member = p_cb_user_data->_p_member;
	char *data = (char *)recv_buffer;
	_u32 data_len = recvd_len;

	DEAL_WITH_LOGIN_RESULT_CALLBACK login_result_callback = p_member->_login_result_callback;

	LOG_DEBUG("query_portal_vip_callback, state=%u, srv_index=%u, addr=%u:%u, result=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, result, p_member->_user_info._userno);

	if(g_querying_vip_portal)
	{
		g_querying_vip_portal = FALSE;
	}
	else
	{
		/* 已被取消 */
		SAFE_DELETE(recv_buffer);
		SAFE_DELETE(p_cb_user_data);
		return SUCCESS;
	}
	
	SAFE_DELETE(p_cb_user_data);
	if (result != SUCCESS)
	{
		if(g_login_state==2 && g_login_vip_state==0)
		{
			// 服务器问题，回调通知。
			if( NULL != login_result_callback )
				ret = login_result_callback(result, NULL, p_member);
		}

		SAFE_DELETE(recv_buffer);
		return SUCCESS;
	}
	
	query_result = member_parse_cmd_query_portal_vip_resp(data, data_len, &p_member->_vip_servers[1],
		MEMBER_SERVER_MAX_COUNT-1, &bak_server_count);

	LOG_DEBUG("query_portal_vip_callback, state=%u, srv_index=%u, addr=%u:%u, query_result=%u, bak_srv_count=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, query_result, bak_server_count, p_member->_user_info._userno);

	SAFE_DELETE(recv_buffer);
	
	if (query_result == SUCCESS && bak_server_count > 0)
	{
		// 开始再查 server。
		p_member->_vip_server_count = bak_server_count + 1;
		if(g_login_state==2 && g_login_vip_state==0)
		{
			p_member->_vip_server_index = 1;
			ret = member_protocal_impl_login_vip(p_member);
			if (ret != SUCCESS)
			{
				// 发送失败了，回调通知。
				if( NULL != login_result_callback )
					ret = login_result_callback(ret, NULL, p_member);
			}
		}
		return SUCCESS;
	}

	if(g_login_state==2 && g_login_vip_state==0)
	{
		// 登录失败了，回调通知。
		if(query_result!=SUCCESS)
		{
			if( NULL != login_result_callback )
				ret = login_result_callback(query_result, NULL, p_member);
		}
		else
		{
			if( NULL != login_result_callback )
				ret = login_result_callback(MEMBER_PROTOCAL_ERROR_QUERY_VIP_PORTAL_FAIL, NULL, p_member);
		}
	}
	return SUCCESS;
}

_int32 member_protocal_impl_ping(MEMBER_PROTOCAL_IMPL * p_member, MEMBER_PROTOCAL_PING * p_member_protocal_ping)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;
	
	if (p_member == NULL || p_member_protocal_ping == NULL)
	{
		return INVALID_ARGUMENT;
	}

	LOG_DEBUG("member_protocal_impl_ping, state=%u, srv_index=%u, addr=%u:%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_user_info._userno);

	if (p_member->_server_index >= p_member->_server_count)
	{
		return MEMBER_PROTOCAL_ERROR_SERVER_FAIL;
	}

	/*
	if (p_member->_state == MEMBER_STATE_PING)
	{
		return SUCCESS;
	}
	else if (p_member->_state == MEMBER_STATE_LOGINED
		|| p_member->_state == MEMBER_STATE_REPORT_DOWNLOAD_FILE)
	{
	}
	else if (p_member->_state == MEMBER_STATE_UNLOGIN
		|| p_member->_state == MEMBER_STATE_LOGINING
		|| p_member->_state == MEMBER_STATE_REFRESHING )
	{
		return MEMBER_PROTOCAL_ERROR_OPER_DENIED;
	}
	else
	{
		sd_assert(-1);
	}
	*/

	ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_PING);
	
	ret = member_build_cmd_ping(p_member_protocal_ping, g_tmp_data, TMP_DATA_SIZE, &cmd_len);
	CHECK_VALUE(ret);

	if (p_member->_server_index == 0 && p_member->_servers[0]._ip == 0)
	{
		ret = member_send_cmd_by_host(p_member->_main_server_host, p_member->_main_server_port,
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_ping_callback);
	}
	else
	{
		ret = member_send_cmd_by_ip(&p_member->_servers[p_member->_server_index],
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_ping_callback);
	}

	if (ret != SUCCESS)
	{
		// 发不出去。
		return MEMBER_PROTOCAL_ERROR_SEND_FAIL;
	}
	return SUCCESS;
}

_int32 member_protocal_impl_ping_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	_int32 query_result = 0;
	MEMBER_PROTOCAL_PING_RESP ping_resp;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL* p_member = p_cb_user_data->_p_member;
	char *data = (char *)recv_buffer;
	_u32 data_len = recvd_len;

	LOG_DEBUG("ping_callback, state=%u, srv_index=%u, addr=%u:%u, result=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, result, p_member->_user_info._userno);

	if (result != SUCCESS)
	{
		// 服务器问题，回调通知。
		ret = member_protocal_interface_ping_resp_impl(MEMBER_PROTOCAL_ERROR_SERVER_FAIL, NULL, p_member);

		if (recv_buffer != SUCCESS)
		{
			ret = sd_free(recv_buffer);
			recv_buffer = NULL;
			CHECK_VALUE(ret);
		}
		return SUCCESS;
	}

	sd_memset(&ping_resp, 0, sizeof(MEMBER_PROTOCAL_PING_RESP));
	query_result = member_parse_cmd_ping_resp(data, data_len, &ping_resp);

	LOG_DEBUG("ping_callback, state=%u, srv_index=%u, addr=%u:%u, query_result=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, query_result, p_member->_user_info._userno);

	ret = member_protocal_interface_ping_resp_impl(query_result, &ping_resp, p_member);

	if (recv_buffer != SUCCESS)
	{
		ret = sd_free(recv_buffer);
		recv_buffer = NULL;
		CHECK_VALUE(ret);
	}
	return SUCCESS;
}

_int32 member_protocal_impl_report_download_file(MEMBER_PROTOCAL_IMPL * p_member,
	MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE * p_member_protocal_rpt_dl_file)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;
	
	if (p_member == NULL || p_member_protocal_rpt_dl_file == NULL)
	{
		return INVALID_ARGUMENT;
	}

	LOG_DEBUG("--- report_download_file, state=%u, srv_index=%u, addr=%u:%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_user_info._userno);

	if (p_member->_server_index >= p_member->_server_count)
	{
		return MEMBER_PROTOCAL_ERROR_SERVER_FAIL;
	}

	/*
	if (p_member->_state == MEMBER_STATE_LOGINED
		|| p_member->_state == MEMBER_STATE_REPORT_DOWNLOAD_FILE
		|| p_member->_state == MEMBER_STATE_PING)
	{
	}
	else if (p_member->_state == MEMBER_STATE_UNLOGIN
		|| p_member->_state == MEMBER_STATE_LOGINING
		|| p_member->_state == MEMBER_STATE_REFRESHING )
	{
		return MEMBER_PROTOCAL_ERROR_OPER_DENIED;
	}
	else
	{
		sd_assert(-1);
	}
	*/

	ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_REPORT_DOWNLOAD_FILE);

	ret = member_build_cmd_report_download_file(p_member_protocal_rpt_dl_file, g_tmp_data, TMP_DATA_SIZE, &cmd_len);
	CHECK_VALUE(ret);

	if (p_member->_server_index == 0 && p_member->_servers[0]._ip == 0)
	{
		ret = member_send_cmd_by_host(p_member->_main_server_host, p_member->_main_server_port,
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_report_download_file_callback);
	}
	else
	{
		ret = member_send_cmd_by_ip(&p_member->_servers[p_member->_server_index],
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_report_download_file_callback);
	}

	if (ret != SUCCESS)
	{
		// 发不出去。
		return MEMBER_PROTOCAL_ERROR_SEND_FAIL;
	}
	return SUCCESS;
}

_int32 member_protocal_impl_report_download_file_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	_int32 query_result = 0;
	MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE_RESP rpt_dl_flie_resp;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL* p_member = p_cb_user_data->_p_member;
	char *data = (char *)recv_buffer;
	_u32 data_len = recvd_len;

	LOG_DEBUG("report_download_file_callback, state=%u, srv_index=%u, addr=%u:%u, result=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, result, p_member->_user_info._userno);

	if (result != SUCCESS)
	{
		// 服务器问题，回调通知。
		ret = member_protocal_interface_report_download_file_resp_impl(MEMBER_PROTOCAL_ERROR_SERVER_FAIL, NULL, p_member);

		if (recv_buffer != SUCCESS)
		{
			ret = sd_free(recv_buffer);
			recv_buffer = NULL;
			CHECK_VALUE(ret);
		}
		return SUCCESS;
	}

	sd_memset(&rpt_dl_flie_resp, 0, sizeof(MEMBER_PROTOCAL_PING_RESP));
	query_result = member_parse_cmd_report_download_file_resp(data, data_len, &rpt_dl_flie_resp);

	LOG_DEBUG("report_download_file_callback, state=%u, srv_index=%u, addr=%u:%u, query_result=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, query_result, p_member->_user_info._userno);

	ret = member_protocal_interface_report_download_file_resp_impl(query_result, &rpt_dl_flie_resp, p_member);

	if (recv_buffer != SUCCESS)
	{
		ret = sd_free(recv_buffer);
		recv_buffer = NULL;
		CHECK_VALUE(ret);
	}
	return SUCCESS;
}

_int32 member_protocal_impl_query_vip_info(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;
	char url[1024] = {0};
	char *buffer = NULL;
	
	LOG_DEBUG("member_protocal_impl_query_vip_info, uid=%llu",  p_member->_user_info._userno);
	
	if(sd_strlen(p_member->_user_info._session_id)==0)
	{	
		LOG_DEBUG("member_protocal_impl_login_vip: sessionid is NULL\n");
		return MEMBER_PROTOCAL_ERROR_NO_SECSSION_ID;
	}
	
	LOG_DEBUG("member_protocal_impl_query_vip_info: userid=%llu, sessionid=%s\n", p_member->_user_info._userno, p_member->_user_info._session_id);

	ret = sd_snprintf(url, sizeof(url), "http://%s:%u/cache?userid=%llu&protocol_version=%d&secure=false&sessionid=%s", MEMBER_VIP_HTTP_SERVER_HOST, MEMBER_VIP_HTTP_SERVER_PORT, p_member->_user_info._userno, QUERY_VIP_PROTOCOL_VERSION, p_member->_user_info._session_id);

	LOG_DEBUG("member_protocal_impl_query_vip_info: url=%s\n", url);
	sd_assert(ret <= 1024);

	ret = sd_malloc(16384, (void**)&buffer);
	if(ret != SUCCESS)
	{
		return ret;
	}

	ret = member_send_cmd_get_buffer(url, buffer, 16384, p_member->_asyn_time_out,p_member, member_protocal_impl_query_vip_info_callback);
	if(ret != SUCCESS)
	{
		SAFE_DELETE(buffer);
		CHECK_VALUE(ret);
	}

	return ret;
}

_int32 member_protocal_impl_query_vip_info_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	_int32 query_result = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL* p_member = p_cb_user_data->_p_member;
	
	char *data = (char *)recv_buffer;
	_u32 data_len = recvd_len;
	_u32 cur_time_stamp = 0;
	char expired_time[] = "未知";
	char utf8_time[8] = {0};
	_u32 utf8_len = 8;
	
	_int32 errcode = 0;

	LOG_DEBUG("member_protocal_impl_query_vip_info_callback,  result=%u, uid=%llu", result, p_member->_user_info._userno);

	SAFE_DELETE(p_cb_user_data);
	errcode = result;
	
	if (result == SUCCESS)
	{
		query_result = member_parse_http_query_vip_resp(data, data_len, &p_member->_user_info);
		// 会员企业账号的子账号强制定义为会员
		if(  p_member->_user_info._vas&0x10000000 )
		{	
		#if defined(CLOUD_PLAY_PROJ) 
			p_member->_user_info._is_vip = TRUE;
			p_member->_user_info._is_platinum = TRUE;
		#endif
            sd_memset(p_member->_user_info._expire_date, 0x00, sizeof(p_member->_user_info._expire_date));
			sd_any_format_to_utf8(expired_time, sd_strlen(expired_time), utf8_time, &utf8_len);
			sd_strncpy(p_member->_user_info._expire_date, utf8_time, utf8_len);
            p_member->_user_info._expire_date_len = utf8_len;
		}

		SAFE_DELETE(recv_buffer);
		if( SUCCESS == query_result)
		{
			member_protocal_interface_refresh_user_info_resp(&p_member->_user_info);
		}
		errcode = query_result;
	}
	else
	{
		SAFE_DELETE(recv_buffer);
	}

	if( NULL != g_refresh_user_info_callback)
		g_refresh_user_info_callback(errcode);
	
	return SUCCESS;
}


_int32 member_protocal_impl_refresh_user_info(MEMBER_PROTOCAL_IMPL * p_member, member_refresh_notify func)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;

	if (p_member == NULL)
	{
		return INVALID_ARGUMENT;
	}

	g_refresh_user_info_callback = func;
	
	LOG_DEBUG("--- member_protocal_impl_refresh_user_info, uid=%llu",p_member->_user_info._userno);

	ret = member_protocal_impl_query_vip_info(p_member);

	return ret;
}

_int32 member_protocal_impl_refresh(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;

	if (p_member == NULL)
	{
		return INVALID_ARGUMENT;
	}
	
	LOG_DEBUG("--- refresh, state=%u, srv_index=%u, addr=%u:%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_user_info._userno);

	if (p_member->_server_index >= p_member->_server_count)
	{
		return MEMBER_PROTOCAL_ERROR_SERVER_FAIL;
	}

	/*
	if (p_member->_state == MEMBER_STATE_REFRESHING)
	{
		return SUCCESS;
	}
	else if (p_member->_state == MEMBER_STATE_UNLOGIN
		|| p_member->_state == MEMBER_STATE_LOGINING)
	{
		return MEMBER_PROTOCAL_ERROR_OPER_DENIED;
	}
	else if (p_member->_state == MEMBER_STATE_LOGINED
		|| p_member->_state == MEMBER_STATE_PING
		|| p_member->_state == MEMBER_STATE_REPORT_DOWNLOAD_FILE)
	{
	}
	else
	{
		sd_assert(-1);
	}
	*/

	ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_REFRESHING);
	
	ret = member_build_cmd_login(&p_member->_login_info, g_tmp_data, TMP_DATA_SIZE, &cmd_len);
	CHECK_VALUE(ret);

	if (p_member->_server_index == 0 && p_member->_servers[0]._ip == 0)
	{
		ret = member_send_cmd_by_host(p_member->_main_server_host, p_member->_main_server_port,
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_login_callback);
	}
	else
	{
		ret = member_send_cmd_by_ip(&p_member->_servers[p_member->_server_index], 
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_login_callback);
	}
	
	if (ret != SUCCESS)
	{
		// 发不出去。
		ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_UNLOGIN);
		return MEMBER_PROTOCAL_ERROR_SEND_FAIL;
	}
	
	return SUCCESS;
}

_int32 member_protocal_impl_refresh_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL * p_member = p_cb_user_data->_p_member;
	//char *data = (char *)recv_buffer;
	//_u32 data_len = recvd_len;

	LOG_DEBUG("refresh, state=%u, srv_index=%u, addr=%u:%u, result=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, result, p_member->_user_info._userno);

	ret = member_protocal_interface_refresh_resp_impl(result, &p_member->_user_info, p_member);

	if (recv_buffer != SUCCESS)
	{
		ret = sd_free(recv_buffer);
		recv_buffer = NULL;
		CHECK_VALUE(ret);
	}
	return SUCCESS;
}

_int32 member_protocal_impl_logout(MEMBER_PROTOCAL_IMPL * p_member,
	MEMBER_PROTOCAL_LOGOUT_INFO * p_member_protocal_logout_info)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;
	
	if (p_member == NULL || p_member_protocal_logout_info == NULL)
	{
		return INVALID_ARGUMENT;
	}

	LOG_DEBUG("--- logout, state=%u, srv_index=%u, addr=%u:%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_user_info._userno);

	if (p_member->_server_index >= p_member->_server_count)
	{
		return MEMBER_PROTOCAL_ERROR_SERVER_FAIL;
	}

	/*
	if (p_member->_state == MEMBER_STATE_UNLOGIN)
	{
		return MEMBER_PROTOCAL_ERROR_OPER_DENIED;
	}
	else if (p_member->_state == MEMBER_STATE_LOGINING
		|| p_member->_state == MEMBER_STATE_LOGINED
		|| p_member->_state == MEMBER_STATE_PING
		|| p_member->_state == MEMBER_STATE_REFRESHING
		|| p_member->_state == MEMBER_STATE_REPORT_DOWNLOAD_FILE)
	{
	}
	else
	{
		sd_assert(-1);
	}
	*/

	ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_UNLOGIN);

	// 登录未完成就退出时，协议内部控制标志g_login_state还是登录状态，需要重置。add by hexiang 2013-12-2
	ret = member_protocal_impl_change_login_flag(0);

	ret = member_build_cmd_logout(p_member_protocal_logout_info, g_tmp_data, TMP_DATA_SIZE, &cmd_len);
	CHECK_VALUE(ret);

	if (p_member->_server_index == 0 && p_member->_servers[0]._ip == 0)
	{
		ret = member_send_cmd_by_host(p_member->_main_server_host, p_member->_main_server_port,
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_logout_callback);
	}
	else
	{
		ret = member_send_cmd_by_ip(&p_member->_servers[p_member->_server_index],
			p_member->_asyn_time_out, g_tmp_data, cmd_len, p_member,
			member_protocal_impl_logout_callback);
	}

	if (ret != SUCCESS)
	{
		// 发不出去。
		ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_UNLOGIN);
		return MEMBER_PROTOCAL_ERROR_SEND_FAIL;
	}
	return SUCCESS;
}

_int32 member_protocal_impl_logout_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	_int32 query_result = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL * p_member = p_cb_user_data->_p_member;
	char *data = (char *)recv_buffer;
	_u32 data_len = recvd_len;

	LOG_DEBUG("logout_callback, state=%u, srv_index=%u, addr=%u:%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_user_info._userno);

	if (result != SUCCESS)
	{
		// 服务器问题，回调通知。
		ret = member_protocal_interface_logout_resp_impl(MEMBER_PROTOCAL_ERROR_SERVER_FAIL, p_member);

		if (ret != SUCCESS)
		{
			ret = sd_free(recv_buffer);
			recv_buffer = NULL;
			CHECK_VALUE(ret);
		}
		return SUCCESS;
	}
	
	query_result = member_parse_cmd_logout_resp(data, data_len);

	LOG_DEBUG("logout_callback, state=%u, srv_index=%u, addr=%u:%u, query_result=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, query_result, p_member->_user_info._userno);

	ret = member_protocal_interface_logout_resp_impl(query_result, p_member);

	if (ret != SUCCESS)
	{
		ret = sd_free(recv_buffer);
		recv_buffer = NULL;
		CHECK_VALUE(ret);
	}
	return SUCCESS;
}

_int32 member_protocal_impl_query_icon(MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = 0;
	char tmp_file_name[MEMBER_PROTOCAL_FILE_NAME_SIZE];
	_u32 file_name_len = 0;
	char* file_path = NULL;
	_int32 file_path_len = 0;

	LOG_DEBUG("query_icon, state=%u, srv_index=%u, addr=%u:%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, p_member->_user_info._userno);

	////将member头像目录初始化移到这里 by zhengzhe
	if(p_member->_user_info._icon._file_path_len == 0)
	{
		file_path = em_get_system_path();
		file_path_len = sd_strlen(file_path);
		if(file_path_len == 0)
		{
			return MEMBER_PROTOCAL_ERROR_PIC_PATH_NOT_EXISTED;
		}
		
		if(!sd_dir_exist(file_path))
		{
#ifndef ENABLE_NEIGHBOR
			sd_assert(FALSE);	
#endif /* NEIGHBOR_PROJ */	
			/* 目录不存在的话不查询头像 */
			return MEMBER_PROTOCAL_ERROR_PIC_PATH_NOT_EXISTED;
		}
		
		ret = sd_memset(p_member->_user_info._icon._file_path, 0, MEMBER_PROTOCAL_FILE_PATH_SIZE);
		CHECK_VALUE(ret);

		p_member->_user_info._icon._file_path_len = file_path_len;
		ret = sd_strncpy(p_member->_user_info._icon._file_path, file_path, file_path_len);
		CHECK_VALUE(ret);

		// 目录名称强制加'\\'
		if (p_member->_user_info._icon._file_path[p_member->_user_info._icon._file_path_len - 1] != '\\'
			&& p_member->_user_info._icon._file_path[p_member->_user_info._icon._file_path_len - 1] != '/')
		{
			p_member->_user_info._icon._file_path[p_member->_user_info._icon._file_path_len ] = DIR_SPLIT_CHAR;
			p_member->_user_info._icon._file_path[p_member->_user_info._icon._file_path_len+1] = '\0';
		}

		// 重新获取长度。
		p_member->_user_info._icon._file_path_len = sd_strlen(p_member->_user_info._icon._file_path);

	}
	////

	member_get_icon_file_name(p_member, tmp_file_name, MEMBER_PROTOCAL_FILE_NAME_SIZE, &file_name_len);
	sd_strcat(tmp_file_name, ".tmp", sd_strlen(".tmp"));

	ret = member_send_cmd_get_file(
		p_member->_user_info._img_url, 
		p_member->_user_info._icon._file_path,
		p_member->_user_info._icon._file_path_len,
		tmp_file_name,
		sd_strlen(tmp_file_name),
		p_member,
		member_protocal_impl_query_icon_callback);
	
	if (ret != SUCCESS)
	{
		// 发不出去。
		return MEMBER_PROTOCAL_ERROR_SEND_FAIL;
	}
	return SUCCESS;
}

_int32 member_protocal_impl_query_icon_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	_int32 ret = 0;
	MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA* p_cb_user_data = (MEMBER_PROTOCAL_SEND_CALLBACK_USER_DATA*)user_data;
	MEMBER_PROTOCAL_IMPL* p_member = p_cb_user_data->_p_member;
	//char *data = (char *)recv_buffer;
	_u32 data_len = recvd_len;

	char tmp_file_name[MEMBER_PROTOCAL_FILE_PATH_SIZE + MEMBER_PROTOCAL_FILE_NAME_SIZE];
	char icon_file_name[MEMBER_PROTOCAL_FILE_PATH_SIZE + MEMBER_PROTOCAL_FILE_NAME_SIZE];
	_u32 file_path_len = 0;
	_u32 file_name_len = 0;

	LOG_DEBUG("query_icon_callback, state=%u, srv_index=%u, addr=%u:%u, result=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, result, p_member->_user_info._userno);

	SAFE_DELETE(p_cb_user_data);
	if(g_login_state!=2)
	{
		//SAFE_DELETE(recv_buffer);
		return SUCCESS;
	}
	
	if (result != SUCCESS)
	{
		// 服务器问题，回调通知。
		//ret = member_protocal_interface_query_icon_resp_impl(MEMBER_PROTOCAL_ERROR_SERVER_FAIL, NULL, p_member);
		return SUCCESS;
	}

	// tmp file name.
	sd_memset(tmp_file_name, 0, 
		sizeof(char) * (MEMBER_PROTOCAL_FILE_PATH_SIZE + MEMBER_PROTOCAL_FILE_NAME_SIZE));
	sd_strncpy(tmp_file_name, p_member->_user_info._icon._file_path, 
		p_member->_user_info._icon._file_path_len);
	file_path_len = p_member->_user_info._icon._file_path_len;

	ret = member_get_icon_file_name(p_member, tmp_file_name + file_path_len,
		MEMBER_PROTOCAL_FILE_PATH_SIZE + MEMBER_PROTOCAL_FILE_NAME_SIZE - file_path_len,
		&file_name_len);
	ret = sd_strcat(tmp_file_name, ".tmp", sd_strlen(".tmp"));

	// icon file name.
	sd_memset(icon_file_name, 0, 
		sizeof(char) * (MEMBER_PROTOCAL_FILE_PATH_SIZE + MEMBER_PROTOCAL_FILE_NAME_SIZE));
	sd_strncpy(icon_file_name, p_member->_user_info._icon._file_path, 
		p_member->_user_info._icon._file_path_len);
	file_path_len = p_member->_user_info._icon._file_path_len;

	ret = member_get_icon_file_name(p_member, p_member->_user_info._icon._file_name,
		MEMBER_PROTOCAL_FILE_NAME_SIZE, &p_member->_user_info._icon._file_name_len);
	ret = sd_strncpy(icon_file_name + file_path_len, 
		p_member->_user_info._icon._file_name,
		p_member->_user_info._icon._file_name_len);

	if (sd_file_exist(icon_file_name))
	{
		ret = sd_delete_file(icon_file_name);
	}
	ret = sd_rename_file(tmp_file_name, icon_file_name);
	p_member->_user_info._icon._file_size = data_len;
	
	ret = member_protocal_interface_query_icon_resp_impl(SUCCESS, &p_member->_user_info._icon, p_member);
	return SUCCESS;
}

_int32 member_protocal_impl_register_check_id_query(MEMBER_PROTOCAL_REGISTER_CHECK_ID *check_id)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;
	char server_url[256] = {0};

	ret = member_build_cmd_register_check_id_query(check_id, g_tmp_data, TMP_DATA_SIZE, &cmd_len);
	CHECK_VALUE(ret);
	
	em_settings_get_str_item("member.register_server_url", server_url);
	if (server_url[0] == '\0')
		sd_strncpy(server_url, MEMBER_REGISTER_SERVER_HOST, sd_strlen(MEMBER_REGISTER_SERVER_HOST));
	
	ret = member_send_cmd_by_url(server_url,
		20, g_tmp_data, cmd_len, (MEMBER_PROTOCAL_IMPL *)check_id,
		member_protocal_impl_register_check_id_query_callback);
	
	if (ret != SUCCESS)
	{
		// 发不出去
		sd_free(check_id);
		check_id = NULL;
		return MEMBER_PROTOCAL_ERROR_SEND_FAIL;
	}
	return SUCCESS;
}

_int32 member_protocal_impl_register_check_id_query_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	MEMBER_PROTOCAL_REGISTER_CHECK_ID *check_id = (MEMBER_PROTOCAL_REGISTER_CHECK_ID*)user_data;
	MEMBER_PROTOCAL_REGISTER_RESULT_INFO result_info;
	_int32 ret = SUCCESS;

	if (result != SUCCESS)
	{
		check_id->_callback_func(MEMBER_PROTOCAL_ERROR_REGISTER_CONNECT_NET_FAILED, check_id->_user_data);
		sd_free(check_id);
		check_id = NULL;
		return -1;
	}

	sd_memset(&result_info, 0x00, sizeof(MEMBER_PROTOCAL_REGISTER_RESULT_INFO));
	
	ret = member_parse_cmd_register_resp(recv_buffer, recvd_len, &result_info);
	if (ret != SUCCESS)
	{
		check_id->_callback_func(MEMBER_PROTOCAL_ERROR_REGISTER_PARSE_PROTOCAL_FAILED, check_id->_user_data);
		sd_free(check_id);
		check_id = NULL;
		return -1;
	}

	if (result_info._result == MEMBER_PROTOCAL_ERROR_REGISTER_SUCCESS)
		result_info._result = SUCCESS;

	check_id->_callback_func(result_info._result, check_id->_user_data);
	sd_free(check_id);
	check_id = NULL;
	
	return SUCCESS;
}

_int32 member_protocal_impl_register_query(MEMBER_PROTOCAL_REGISTER_INFO *register_info, BOOL is_utf8)
{
	_int32 ret = 0;
	_u32 cmd_len = 0;
	char server_url[256] = {0};

	ret = member_build_cmd_register_query(register_info, g_tmp_data, TMP_DATA_SIZE, &cmd_len, is_utf8);
	CHECK_VALUE(ret);

	em_settings_get_str_item("member.register_server_url", server_url);
	if (server_url[0] == '\0')
		sd_strncpy(server_url, MEMBER_REGISTER_SERVER_HOST, sd_strlen(MEMBER_REGISTER_SERVER_HOST));

	ret = member_send_cmd_by_url(MEMBER_REGISTER_SERVER_HOST,
		20, g_tmp_data, cmd_len, (MEMBER_PROTOCAL_IMPL *)register_info,
		member_protocal_impl_register_query_callback);
	
	if (ret != SUCCESS)
	{
		// 发不出去
		sd_free(register_info);
		register_info = NULL;
		return MEMBER_PROTOCAL_ERROR_SEND_FAIL;
	}
	return SUCCESS;
}

_int32 member_protocal_impl_register_query_callback(void * user_data, _int32 result, void * recv_buffer, _u32 recvd_len)
{
	MEMBER_PROTOCAL_REGISTER_INFO *register_info = (MEMBER_PROTOCAL_REGISTER_INFO*)user_data;
	MEMBER_PROTOCAL_REGISTER_RESULT_INFO result_info;
	_int32 ret = SUCCESS;

	if (result != SUCCESS)
	{
		register_info->_callback_func(MEMBER_PROTOCAL_ERROR_REGISTER_CONNECT_NET_FAILED, register_info->_user_data);
		sd_free(register_info);
		register_info = NULL;
		return -1;
	}

	sd_memset(&result_info, 0x00, sizeof(MEMBER_PROTOCAL_REGISTER_RESULT_INFO));
	
	ret = member_parse_cmd_register_resp(recv_buffer, recvd_len, &result_info);
	if (ret != SUCCESS)
	{
		register_info->_callback_func(MEMBER_PROTOCAL_ERROR_REGISTER_PARSE_PROTOCAL_FAILED, register_info->_user_data);
		sd_free(register_info);
		register_info = NULL;
		return -1;
	}

	if (result_info._result == MEMBER_PROTOCAL_ERROR_REGISTER_SUCCESS)
		result_info._result = SUCCESS;
	register_info->_callback_func(result_info._result, register_info->_user_data);
	sd_free(register_info);
	register_info = NULL;
	
	return SUCCESS;
}


// ------------------------------------------------------------------------
_int32 member_protocal_interface_login_basic_resp_impl(MEMBER_PROTOCAL_USER_INFO * p_user_info)
{
	LOG_DEBUG("member_protocal_interface_login_basic_resp_impl, uid=%llu,name[%u]=%s,new_no=%llu,type=%d,session_id[%u]=%s,jumpkey[%u]=%s,img_url[%u]=%s", 
		p_user_info->_userno,p_user_info->_user_name_len,p_user_info->_user_name,p_user_info->_user_new_no,p_user_info->_user_type,
		p_user_info->_session_id_len,p_user_info->_session_id,p_user_info->_jumpkey_len,p_user_info->_jumpkey,p_user_info->_img_url_len,p_user_info->_img_url);
	return member_protocal_interface_login_basic_resp(p_user_info);
}

_int32 member_protocal_interface_login_resp_impl(_int32 error_code, 
	MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info, MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = SUCCESS;

	LOG_DEBUG("member_protocal_interface_login_resp_impl, state=%u, srv_index=%u, addr=%u:%u, error_code=%d, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, error_code, p_member->_user_info._userno);
	if(g_querying_portal)
	{
		/* 取消查询 */
		g_querying_portal = FALSE;
	}
	
	if(g_querying_vip_portal)
	{
		/* 取消查询 */
		g_querying_vip_portal = FALSE;
	}

	g_login_start_time_stamp = 0;
	g_login_max_time_out = 0;
	
	if(error_code!=SUCCESS&&p_member->_server_index!=0)
	{
		// 由于发现候选服务器老是不靠谱，强制再次初始化服务器个数。
		//p_member->_server_count = 1;
		p_member->_server_index = 0;
		p_member->_servers[0]._ip = 0;
		//p_member->_vip_server_count = 1;
		p_member->_vip_server_index = 0;
		p_member->_vip_servers[0]._ip = 0;
	}
	
	if (p_member->_state == MEMBER_STATE_REFRESHING)
	{
		return member_protocal_interface_refresh_resp_impl(error_code, 
			p_member_protocal_user_info, p_member);
	}
	
	if (error_code == SUCCESS)
	{
		ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_LOGINED);
		CHECK_VALUE(ret);
	}
	else
	{
		ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_UNLOGIN);
		CHECK_VALUE(ret);
	}
	return member_protocal_interface_login_resp(error_code, p_member_protocal_user_info);
}

_int32 member_protocal_interface_sessionid_login_resp_impl(_int32 error_code, 
	MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info, MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = SUCCESS;

	LOG_DEBUG("member_protocal_interface_sessionid_login_resp_impl, state=%u, srv_index=%u, addr=%u:%u, error_code=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, error_code, p_member->_user_info._userno);
	if(g_querying_portal)
	{
		/* 取消查询 */
		g_querying_portal = FALSE;
	}
	
	if(g_querying_vip_portal)
	{
		/* 取消查询 */
		g_querying_vip_portal = FALSE;
	}

	g_login_start_time_stamp = 0;
	g_login_max_time_out = 0;
	
	if(error_code!=SUCCESS&&p_member->_server_index!=0)
	{
		// 由于发现候选服务器老是不靠谱，强制再次初始化服务器个数。
		//p_member->_server_count = 1;
		p_member->_server_index = 0;
		p_member->_servers[0]._ip = 0;
		//p_member->_vip_server_count = 1;
		p_member->_vip_server_index = 0;
		p_member->_vip_servers[0]._ip = 0;
	}
	
	if (error_code == SUCCESS)
	{
		ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_LOGINED);
		CHECK_VALUE(ret);
	}
	else
	{
		ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_UNLOGIN);
		CHECK_VALUE(ret);
	}
	return member_protocal_interface_sessionid_login_resp(error_code, p_member_protocal_user_info);
}

_int32 member_protocal_interface_ping_resp_impl(_int32 error_code,
	MEMBER_PROTOCAL_PING_RESP *p_member_protocal_ping_resp, MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = SUCCESS;

	LOG_DEBUG("interface_ping_resp, state=%u, srv_index=%u, addr=%u:%u, error_code=%u, kick=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, error_code, p_member_protocal_ping_resp->_should_kick, p_member->_user_info._userno);

	if (error_code == SUCCESS
		&& (p_member_protocal_ping_resp->_should_kick == 1
		|| p_member_protocal_ping_resp->_should_kick == 2))
	{
		// 被踢或超时了。
		//sd_assert(FALSE); /* 尽量避免被踢,但不应该出现超时 */
		ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_UNLOGIN);
		CHECK_VALUE(ret);
	}
	else
	{
		ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_LOGINED);
		CHECK_VALUE(ret);
	}
	return member_protocal_interface_ping_resp(error_code, p_member_protocal_ping_resp);
}

_int32 member_protocal_interface_refresh_resp_impl(_int32 error_code,
	MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info, MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = SUCCESS;

	LOG_DEBUG("interface_refresh_resp, state=%u, srv_index=%u, addr=%u:%u, error_code=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, error_code, p_member->_user_info._userno);

	if (error_code == SUCCESS)
	{
		ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_LOGINED);
		CHECK_VALUE(ret);
	}
	else
	{
		ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_UNLOGIN);
		CHECK_VALUE(ret);
	}
	return member_protocal_interface_refresh_resp(error_code, p_member_protocal_user_info);
}

_int32 member_protocal_interface_logout_resp_impl(_int32 error_code, MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = SUCCESS;

	LOG_DEBUG("interface_logout_resp, state=%u, srv_index=%u, addr=%u:%u, error_code=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, error_code, p_member->_user_info._userno);

	if(p_member->_server_index!=0)
	{
		// 由于发现候选服务器老是不靠谱，强制再次初始化服务器个数。
		//p_member->_server_count = 1;
		p_member->_server_index = 0;
		p_member->_servers[0]._ip = 0;
		//p_member->_vip_server_count = 1;
		p_member->_vip_server_index = 0;
		p_member->_vip_servers[0]._ip = 0;
	}
	
	ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_UNLOGIN);
	return member_protocal_interface_logout_resp(error_code);
}

_int32 member_protocal_interface_query_icon_resp_impl(_int32 error_code, MEMBER_PROTOCAL_ICON * p_icon,
	MEMBER_PROTOCAL_IMPL * p_member)
{
	// 暂不改变状态。

	LOG_DEBUG("interface_query_icon_resp, state=%u, srv_index=%u, addr=%u:%u, error_code=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, error_code, p_member->_user_info._userno);

	return member_protocal_interface_query_icon_resp(error_code, p_icon);
}

_int32 member_protocal_interface_report_download_file_resp_impl(_int32 error_code, 
	MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE_RESP * p_rpt_dl_file_resp, MEMBER_PROTOCAL_IMPL * p_member)
{
	_int32 ret = SUCCESS;

	LOG_DEBUG("interface_report_download_file_resp, state=%u, srv_index=%u, addr=%u:%u, error_code=%u, uid=%llu", p_member->_state,
		p_member->_server_index, p_member->_servers[p_member->_server_index]._ip,
		p_member->_servers[p_member->_server_index]._port, error_code, p_member->_user_info._userno);

	ret = member_protocal_impl_state_change(p_member, MEMBER_STATE_LOGINED);
	CHECK_VALUE(ret);

	return member_protocal_interface_report_download_file_resp(error_code, p_rpt_dl_file_resp);
}

// ------------------------------------------------------------------------
#endif /* ENABLE_MEMBER */

