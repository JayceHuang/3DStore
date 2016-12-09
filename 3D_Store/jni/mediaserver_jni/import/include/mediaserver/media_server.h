#ifndef MEDIA_SERVER_H_
#define MEDIA_SERVER_H_

enum MEDIA_SERVER_ERRCODE
{
    MEDIA_SERVER_E_UNKNOW        = -1,
    MEDIA_SERVER_E_SUCCESS       = 0,
    MEDIA_SERVER_E_NOT_INIT      = 1,
    MEDIA_SERVER_E_ALREADY_INIT  = 2,
    MEDIA_SERVER_E_INIT_FAILED   = 3,
	MEDIA_SERVER_E_PARAMETER_ERROR   = 4,
};



//************************************
// Method:    media_server_init
// MediaServer��ʼ��
// Returns:   int						�ɹ�����MEDIA_SERVER_E_SUCCESS
//                                      ʧ�ܷ���MEDIA_SERVER_E_INIT_FAILED, 
//                                              MEDIA_SERVER_E_ALREADY_INIT
// Parameter: int appid					Ӧ��ID(ϸ���ӿ��ĵ�����appId���)
// Parameter: int port					MediaServer�����˿�
// Parameter: const char * root			MediaServer����Ŀ¼
// Note: ִ��media_server_initʱ���������һ�ε���media_server_init��
//       ��Ҫ��֤������media_server_uninit����media_server_run�Ѿ�����,
//       ���򷵻�MEDIA_SERVER_E_ALREADY_INIT�Ĵ���
//************************************
int media_server_init(int appid, int port, const char *root);

//************************************
// Method:    media_server_set_local_info
// ���ñ�����Ϣ����media_server_init���óɹ��󣬺ͱ�����Ϣ�б仯ʱ���ã�
// Returns:   int						�ɹ�����MEDIA_SERVER_E_SUCCESS
// Parameter: char * peed_id		
// Parameter: char * imei			
// Parameter: char * device_type		Ʒ��|�ͺţ��硰Sony|Lt26i��
// Parameter: char * os_version			����ϵͳ�汾��
// Parameter: char * net_type			������ʽ��null/2g/3g/4g/wifi/other��
//************************************
int media_server_set_local_info(const char *peed_id, const char *imei, const char *device_type, const char* os_version, const char *net_type);



//************************************
// Method:    media_server_uninit
// MediaServer����ʼ��
// Returns:   void
//************************************
void media_server_uninit();
//************************************
// Method:    media_server_run
// MediaServer�������������̵߳�run������
// ��Ҫʹ�����ṩ�߳���ִ�С����û�е���media_server_init��
// ��������MEDIA_SERVER_E_NOT_INITʧ�ܡ������̻߳�ִ��ֱ������media_server_uninit
// ����ֹ�̣߳�����MEDIA_SERVER_E_SUCCESS��
// ����media_server_run�󣬻�ȵ�media_server_uninit�����ú󣬲ŷ��ء�
// Returns:   int      �ɹ����� MEDIA_SERVER_E_SUCCESS
//                     ʧ�ܷ��� MEDIA_SERVER_E_NOT_INIT
//************************************
int media_server_run();
//************************************
// Method:    media_server_get_http_listen_port
// ��ȡMediaServer�����˿�
// Returns:   int				�ɹ�����listen_port��ʧ�ܷ���0
//************************************
int media_server_get_http_listen_port();

#endif  /* MEDIA_SERVER_H_ */

