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
// ���������

// δ��ʼ��
#define MEMBER_PROTOCAL_ERROR_UNINIT                        (-101)

// δ��¼
#define MEMBER_PROTOCAL_ERROR_UNLOGIN                       (-102)

// �޷����Ϸ�������
#define MEMBER_PROTOCAL_ERROR_SERVER_FAIL                   (-103)

// ���������ش�������ݰ���
#define MEMBER_PROTOCAL_ERROR_SERVER_UNKNOW_RESP            (-104)

// ��������
#define MEMBER_PROTOCAL_ERROR_SEND_FAIL                     (-105)

// ����������
#define MEMBER_PROTOCAL_ERROR_OPER_DENIED                   (-106)

// ����
#define MEMBER_PROTOCAL_ERROR_KICK_OUT                      (-107)

// ��ʱ������
#define MEMBER_PROTOCAL_ERROR_NEED_RETRY                    (-108)

//ͼƬ·��������
#define MEMBER_PROTOCAL_ERROR_PIC_PATH_NOT_EXISTED	(-109)

// ����login ������������ʧ����
#define MEMBER_PROTOCAL_ERROR_SERVER_ALL_FAIL                   (-110)
#define MEMBER_PROTOCAL_ERROR_SEND_LOGIN_FAIL                     (-111)
#define MEMBER_PROTOCAL_ERROR_SEND_LOGIN_PORTAL_FAIL                    (-112)
#define MEMBER_PROTOCAL_ERROR_QUERY_LOGIN_PORTAL_FAIL                    (-113)
#define MEMBER_PROTOCAL_ERROR_SERVER_ALL_VIP_FAIL                   (-114)
#define MEMBER_PROTOCAL_ERROR_NO_SECSSION_ID                   (-115)
#define MEMBER_PROTOCAL_ERROR_QUERY_VIP_PORTAL_FAIL                    (-116)

#define MEMBER_PROTOCAL_ERROR_LOGIN_USE_CACHE                    			(-200)
#define MEMBER_PROTOCAL_ERROR_LOGIN_SESSIONID_UPDATED                    (-201)

// �ʺŵ�¼�������붨�壺
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

//�ʺ�ע�ᣬ�����붨��
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


//��¼��Ϣ��
typedef struct tag_MEMBER_PROTOCAL_LOGIN_INFO
{
	char _user_name[MEMBER_PROTOCAL_USER_NAME_SIZE];
	_u32 _user_name_len;
	_u32 _login_type;	//	0: ʹ��username/usernewno/useremail��¼	1: ʹ��userno��¼
	_u32 _business_type;
	char _passwd[MEMBER_PROTOCAL_PASSWD_SIZE];	// δ���ܡ�
	_u32 _passwd_len;
	char _peerid[MEMBER_PROTOCAL_PEER_ID_SIZE];
	_u32 _peerid_len;
}MEMBER_PROTOCAL_LOGIN_INFO;

//sessionid��¼��Ϣ��
typedef struct tag_MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO
{
	char _sessionid[MEMBER_PROTOCAL_WEB_SESSION_ID_SIZE];
	_u32 _sessionid_len;
	_u16 _sessionid_type;	//	0: ҳ��	1: �ͻ��� (sessionid����Դ����)
	_u8  _business_type;
	char _peerid[MEMBER_PROTOCAL_PEER_ID_SIZE];
	_u32 _peerid_len;
	_u64 _origin_user_id;
	_u8  _origin_business_type;
}MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO;

// ͷ����Ϣ��
typedef struct tag_MEMBER_PROTOCAL_ICON
{
	char _file_path[MEMBER_PROTOCAL_FILE_PATH_SIZE];
	_u32 _file_path_len;
	char _file_name[MEMBER_PROTOCAL_FILE_NAME_SIZE];
	_u32 _file_name_len;	// 0: û�л�ȡ��ͷ��
	_u64 _file_size;
}MEMBER_PROTOCAL_ICON;

// ��Ա��Ϣ��
typedef struct tag_MEMBER_PROTOCAL_USER_INFO
{
	_u64 _userno;	// �û�ΨһID
	char _user_name[MEMBER_PROTOCAL_USER_NAME_SIZE];	// �û�����ĸ�ʺ�
	_u32 _user_name_len;
	_u64 _user_new_no;	// �û��������ʺ�
	_u8 _user_type;	// 1�����ʺ�  2�����ʺ�
	char _session_id[MEMBER_PROTOCAL_SESSION_ID_SIZE];
	_u32 _session_id_len;
	char _jumpkey[MEMBER_PROTOCAL_JUMPKEY_MAX_SIZE];
	_u32 _jumpkey_len;
	char _img_url[MEMBER_PROTOCAL_IMG_URL_SIZE];
	_u32 _img_url_len;

	// �û�����	
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

	// vip �����Ϣ
	_u8 _is_vip;	// �Ƿ�VIP
	_u8 _is_year;  // �Ƿ���ѻ�Ա
	_u32 _is_platinum; //�Ƿ�׽��Ա
	_u32 _vas;	// (�ɲ���)
	_u16 _level;	// VIP�ȼ�(��VIP�ɳ�ֵ���)
	_u32 _grow;	// VIP�ɳ�ֵ
	_u32 _payid;	// VIP֧������(��ע�����¼�������, 5:"���"�� 3:"�ֻ�����",  >1000:"�����Ա" )
	char _payid_name[MEMBER_PROTOCAL_PAYID_NAME_SIZE];	// VIP֧����������
	_u32 _payid_name_len;
	char _expire_date[MEMBER_PROTOCAL_DATE_SIZE];	// 	��������
	_u32 _expire_date_len;
	_u8 _is_auto_deduct;	// �Ƿ��Զ�����(�ɲ���)
	_u8 _is_remind;	// �Ƿ�����(�ɲ���)
	_u32 _daily;	  // VIPÿ�ճɳ�ֵ(��֧�����;��������У����:15/�գ� �ֻ�:5/�գ� ����:0/�ա������5<daily<15��)
	_u32 _vas_type; //��Ա����: 2-��ͨ��Ա 3-�׽��Ա 4--��ʯ��Ա
	// ͷ��
	MEMBER_PROTOCAL_ICON _icon;
}MEMBER_PROTOCAL_USER_INFO;

// ������Ϣ��
typedef struct tag_MEMBER_PROTOCAL_PING
{
	_u64 _userno;
	char _session_id[MEMBER_PROTOCAL_SESSION_ID_SIZE];
	_u32 _session_id_len;
}MEMBER_PROTOCAL_PING;

// ������ϢӦ��
typedef struct tag_MEMBER_PROTOCAL_PING_RESP
{
	_u64 _userno;
	_u8 _should_kick;	//	0����������; 1�����ˣ�����������; 2����ʱ����Ҫ������
}MEMBER_PROTOCAL_PING_RESP;

// �ϱ������ļ���
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

// �ϱ������ļ�Ӧ��
typedef struct tag_MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE_RESP
{
	_u32 _today_score;	// ����÷֣�����ʱ�����+�ļ����֣�
	_u32 _df_score;	// �������ص÷�
	_u32 _total_score;	// �û��ܷ֣���ʷ�������ܷ֣�
	_u32 _allow_score;	// ���컹�������ӷ���
}MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE_RESP;

// ע����Ϣ��
typedef struct tag_MEMBER_PROTOCAL_LOGOUT_INFO
{
	_u64 _userno;
	char _session_id[MEMBER_PROTOCAL_SESSION_ID_SIZE];
	_u32 _session_id_len;
}MEMBER_PROTOCAL_LOGOUT_INFO;

//���user name��Ϣ
typedef struct tagMEMBER_PROTOCAL_REGISTER_CHECK_ID
{
	void* _user_data;
	member_register_callback _callback_func;
	char _userid[MEMBER_PROTOCAL_USER_NAME_SIZE];
	_int32 _usertype;
}MEMBER_PROTOCAL_REGISTER_CHECK_ID;

//ע����Ϣ
typedef struct tagMEMBER_PROTOCAL_REGISTER_INFO
{	
	void         *_user_data;
	member_register_callback _callback_func;

	char         _userid[MEMBER_PROTOCAL_USER_NAME_SIZE];
	_int32       _usertype;										//�û�����0::���ʺ�,1:���ʺţ�Ĭ��Ϊ0
	char         _password[MEMBER_PROTOCAL_PASSWD_SIZE];
	_int32       _registertype;									//ע������0:����ע��,1:Ѹ��ע��,2:ipad
	char         _nickname[MEMBER_PROTOCAL_USER_NAME_SIZE];
}MEMBER_PROTOCAL_REGISTER_INFO;

typedef struct tagMEMBER_PROTOCAL_REGISTER_RESULT_INFO
{
	char        _command_id[64];
	_u32       _result;
}MEMBER_PROTOCAL_REGISTER_RESULT_INFO;

// ------------------------------------------------------------------------

//��ȡ��������͡�
const char * member_protocal_interface_error_info(_int32 error_code);

// ------------------------------------------------------------------------

// ��ʼ����
_int32 member_protocal_interface_init(char *file_path, _u32 file_path_len);

// ����ʼ����
_int32 member_protocal_interface_uninit(void);

// �û���¼��
_int32 member_protocal_interface_first_login(MEMBER_PROTOCAL_LOGIN_INFO * p_member_protocal_login_info);
_int32 member_protocal_interface_login(MEMBER_PROTOCAL_LOGIN_INFO * p_member_protocal_login_info);

// sessionid��¼��
_int32 member_protocal_interface_sessionid_login(MEMBER_PROTOCAL_SESSIONID_LOGIN_INFO * p_member_protocal_sessionid_login_info);

// �û���¼ʱ�Ļ������ݻص�
_int32 member_protocal_interface_login_basic_resp( MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info);

// �û���¼�ص���
// error_code == SUCCESS, �ɹ���
// error_code != SUCCESS, ʧ�ܣ���ʱ p_member_protocal_user_info ΪNULL��
// �麯���������ɵ���ģ��ʵ�ֶ��塣
_int32 member_protocal_interface_login_resp(_int32 error_code, MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info);

// �û��ϱ�������Ϣ��
_int32 member_protocal_interface_ping(MEMBER_PROTOCAL_PING * p_member_protocal_ping);

// �û��ϱ�������Ϣ�ص���
// error_code == SUCCESS, �ɹ���
// error_code != SUCCESS, ʧ�ܣ���ʱ p_member_protocal_ping_resp ΪNULL��
// �麯���������ɵ���ģ��ʵ�ֶ��塣
_int32 member_protocal_interface_ping_resp(_int32 error_code, MEMBER_PROTOCAL_PING_RESP *p_member_protocal_ping_resp);

// �ϱ������ļ���
_int32 member_protocal_interface_report_download_file(MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE * p_member_protocal_rpt_dl_file);

// �ϱ������ļ��ص���
// error_code == SUCCESS, �ɹ���
// error_code != SUCCESS, ʧ�ܣ���ʱ p_member_protocal_ping_resp ΪNULL��
// �麯���������ɵ���ģ��ʵ�ֶ��塣
_int32 member_protocal_interface_report_download_file_resp(_int32 error_code, 
	MEMBER_PROTOCAL_REPORT_DOWNLOAD_FILE_RESP *p_member_protocal_rpt_dl_file_resp);

// �û���Ϣˢ�¡�
_int32 member_protocal_interface_refresh(MEMBER_PROTOCAL_LOGIN_INFO * p_member_protocal_login_info);

// �û���Ϣˢ�»ص���
// error_code == SUCCESS, �ɹ���
// error_code != SUCCESS, ʧ�ܣ���ʱ p_member_protocal_user_info ΪNULL��
// �麯���������ɵ���ģ��ʵ�ֶ��塣
_int32 member_protocal_interface_refresh_resp(_int32 error_code, MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info);

// �û�ע����
_int32 member_protocal_interface_logout(MEMBER_PROTOCAL_LOGOUT_INFO * p_member_protocal_logout_info);

_int32 member_protocal_interface_change_login_type(_u32 type);

//����û�ID
_int32 member_protocal_interface_register_check_id(MEMBER_PROTOCAL_REGISTER_CHECK_ID*check_id);

//ע���û�
_int32 member_protocal_interface_register(MEMBER_PROTOCAL_REGISTER_INFO *register_info, BOOL is_utf8);

// �û�ע���ص���
// error_code == SUCCESS, �ɹ���
// error_code != SUCCESS, ʧ�ܡ�
// �麯���������ɵ���ģ��ʵ�ֶ��塣
_int32 member_protocal_interface_logout_resp(_int32 error_code);

// ��ѯͷ��ص���
// error_code == SUCCESS, �ɹ���
// error_code != SUCCESS, ʧ�ܣ���ʱ p_icon ΪNULL��
// �麯���������ɵ���ģ��ʵ�ֶ��塣
_int32 member_protocal_interface_query_icon_resp(_int32 error_code, MEMBER_PROTOCAL_ICON * p_icon);

/* user info �п�����login֮��Ż�� */
_int32 member_protocal_interface_user_info_resp(_int32 errcode, MEMBER_PROTOCAL_USER_INFO * p_member_protocal_user_info);

_int32 member_protocal_interface_refresh_user_info(member_refresh_notify func);

// ------------------------------------------------------------------------
#endif /* ENABLE_MEMBER */

#ifdef __cplusplus
}
#endif

#endif

