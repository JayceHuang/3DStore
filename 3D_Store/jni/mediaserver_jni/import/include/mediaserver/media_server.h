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
// MediaServer初始化
// Returns:   int						成功返回MEDIA_SERVER_E_SUCCESS
//                                      失败返回MEDIA_SERVER_E_INIT_FAILED, 
//                                              MEDIA_SERVER_E_ALREADY_INIT
// Parameter: int appid					应用ID(细见接口文档后面appId表格)
// Parameter: int port					MediaServer监听端口
// Parameter: const char * root			MediaServer配置目录
// Note: 执行media_server_init时，如果是再一次调用media_server_init，
//       需要保证调用了media_server_uninit并且media_server_run已经返回,
//       否则返回MEDIA_SERVER_E_ALREADY_INIT的错误。
//************************************
int media_server_init(int appid, int port, const char *root);

//************************************
// Method:    media_server_set_local_info
// 设置本地信息（在media_server_init调用成功后，和本地信息有变化时设置）
// Returns:   int						成功返回MEDIA_SERVER_E_SUCCESS
// Parameter: char * peed_id		
// Parameter: char * imei			
// Parameter: char * device_type		品牌|型号，如“Sony|Lt26i”
// Parameter: char * os_version			操作系统版本号
// Parameter: char * net_type			联网方式（null/2g/3g/4g/wifi/other）
//************************************
int media_server_set_local_info(const char *peed_id, const char *imei, const char *device_type, const char* os_version, const char *net_type);



//************************************
// Method:    media_server_uninit
// MediaServer反初始化
// Returns:   void
//************************************
void media_server_uninit();
//************************************
// Method:    media_server_run
// MediaServer主函数（类似线程的run函数）
// 需要使用者提供线程来执行。如果没有调用media_server_init，
// 立即返回MEDIA_SERVER_E_NOT_INIT失败。正常线程会执行直到调用media_server_uninit
// 来终止线程，返回MEDIA_SERVER_E_SUCCESS。
// 调用media_server_run后，会等到media_server_uninit被调用后，才返回。
// Returns:   int      成功返回 MEDIA_SERVER_E_SUCCESS
//                     失败返回 MEDIA_SERVER_E_NOT_INIT
//************************************
int media_server_run();
//************************************
// Method:    media_server_get_http_listen_port
// 获取MediaServer监听端口
// Returns:   int				成功返回listen_port，失败返回0
//************************************
int media_server_get_http_listen_port();

#endif  /* MEDIA_SERVER_H_ */

