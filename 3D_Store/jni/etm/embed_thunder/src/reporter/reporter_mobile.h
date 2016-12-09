#ifndef _REPORTER_MOBILE_H_20100420_1455_
#define _REPORTER_MOBILE_H_20100420_1455_
#if defined(MOBILE_PHONE)

#ifdef __cplusplus
extern "C" 
{
#endif

/****************************************************/

#include "utility/define.h"

/****************************************************/

// �û���¼��Ϊ����ӦֵΪ:
// 1-��װӦ�ó���
// 2-��Ӧ�ó���
// 3-�˳�Ӧ�ó���
#define REPORTER_USER_ACTION_TYPE_LOGIN (0)

// ��������Ϊ����ӦֵΪ:
// 1-��
// 2-�˳�
#define REPORTER_USER_ACTION_TYPE_PLAYER (1)

// TEXT��Ϊ����ӦֵΪ:
// 1-mov
// 2-text
// 3-software
// 9-other
#define REPORTER_USER_ACTION_TYPE_DOWNLOAD_FILE (2)

//��������������Ϊ
// 1-��ʾ���뿴��������
#define REPORTER_USER_ACTION_ENTER_KANKAN_PLAYER   (3)

//����������������Ϣ
//11001-��ʾremark(data)�ֶδ�ŵ���KANKAN_PLAY_INFO_STAT����Э���
//12001-��ʾremark(data)�ֶδ�ŵ���KANKAN_USER_ACTION_STAT����Э���
#define REPORTER_USER_ACTION_PLAY_INFO    (999)

#define KANKAN_PLAY_INFO_STAT_CMD_TYPE    (11001)
#define KANKAN_USER_ACTION_STAT_CMD_TYPE    (11003)

/****************************************************/

// ��ʼ��
_int32 reporter_init(const char *record_dir, _u32 record_dir_len);

// ����ʼ��
_int32 reporter_uninit();

// ����peerid.
_int32 reporter_set_peerid(char *peer_id, _u32 size);

// ����ver.
_int32 reporter_set_version(const char *ver, _int32 size,_int32 product,_int32 partner_id);

//����װ���߿�������Ϊ��¼���ϱ��ļ���
_int32 reporter_new_install(BOOL is_install);

// ���û�����Ϊ��¼���ļ��С�
_int32 reporter_mobile_user_action_to_file(_u32 action_type, _u32 action_value, void *data, _u32 data_len);

_int32 reporter_set_version_handle(void * _param);

_int32 get_ui_version(char *buffer, _int32 bufsize);

_int32 reporter_new_install_handle(void * _param);

_int32 reporter_mobile_user_action_to_file_handle( void * _param );

//timer �ϱ��û���Ϊ��¼�ļ��Ŀ��أ�Ĭ�Ͽ���
_int32 reporter_mobile_enable_user_action_report_handle(void * _param);

// ������������ʱ���ϱ�������Ϣ��
_int32 reporter_mobile_network(void);

// ���ļ��ж�ȡͳ����Ϣ�����ϱ���
_int32 reporter_from_file();

// �ϱ��ɹ��ص�������
_int32 reporter_from_file_callback();

/****************************************************/

#ifdef __cplusplus
}
#endif

#endif
#endif
